--------------------------------------------------------------
-- $Id: prc_shotpoint.sql,v 1.1.1.1 2007/04/24 02:04:25 rob Exp $
-- $Name:  $
--------------------------------------------------------------
-- Usage   :       select spt2( gid, geometry));
-- Example :       
--------------------------------------------------------------
-- CREATE TYPE shotpoint AS
-- (gid integer, sequence_num integer, station_num varchar, angle integer,the_geom geometry);
--    Consider adding the following:
--             ind_position char { A=end point, B=bend point }
--             elevation double precision
--             oid (build using gid*10000 + sequence_num)

CREATE OR REPLACE
    FUNCTION spt2(integer, integer, geometry)
    RETURNS SETOF shotpoint AS '
DECLARE
    
    v_gid        ALIAS FOR $1;
    v_interval   ALIAS FOR $2;
    v_geom       ALIAS FOR $3;
    v_shotpt     shotpoint%ROWTYPE;

    v_sta1     seismic_2d_sp.station_num%TYPE;
    v_sta2     seismic_2d_sp.station_num%TYPE;
    v_seq1     seismic_2d_sp.sequence_num%TYPE;
    v_seq2     seismic_2d_sp.sequence_num%TYPE;
    v_sta_rec  RECORD;
    v_snum     integer;
    v_numpts   integer;
    v_lastidx  integer = -1;

    -- For later: v_smp_geom geometry;   -- Simplified geometry that will contain bend points
    -- For later: v_nsmp     integer;    -- Number of points in simplified geometry

    i          integer;
    n          integer;
    ichr       integer;
    is_invalid boolean;

BEGIN

    v_numpts = NumPoints(v_geom);
        -- Get decimated sequence of stations
        FOR v_sta_rec in select gid,sequence_num,station_num
            from seismic_2d_sp
            where gid=v_gid ORDER BY sequence_num LOOP

            -- See if station_num is numeric (It is possible to have something like 960A)
            is_invalid = false;
            n = length(v_sta_rec.station_num);
            FOR i in 1 .. n LOOP
                ichr = ascii(substr(v_sta_rec.station_num, i, 1));
                if ichr < 48 or ichr > 57 then
                    is_invalid = true;
                    EXIT;   -- exit loop
                end if;
            END LOOP;

            if not is_invalid then
                v_snum = cast(v_sta_rec.station_num as numeric);
                if    v_sta_rec.sequence_num = 0
                   or v_sta_rec.sequence_num = v_numpts-1
                   or mod(v_snum,v_interval) = 0 then

                    v_shotpt.gid           = v_gid;
                    v_shotpt.sequence_num  = v_sta_rec.sequence_num;
                    v_shotpt.station_num   = v_sta_rec.station_num;
                    v_shotpt.elevation     = NULL;
                    v_shotpt.angle
                          = ComputeLabelAngle(v_lastidx, v_sta_rec.sequence_num, v_geom);
                    v_shotpt.indicator     = NULL;
                    v_shotpt.the_geom      = PointN(v_geom, v_sta_rec.sequence_num + 1);
            
                    return next v_shotpt;
                end if;
            end if;
            v_lastidx = v_sta_rec.sequence_num;
        END LOOP;

    return;
END;
'
language 'plpgsql';
-- #########################################################################
CREATE OR REPLACE
    FUNCTION shotpoints(integer)
    RETURNS SETOF shotpoint AS '
DECLARE
    v_interval   ALIAS FOR $1;
    v_shotpt     shotpoint%ROWTYPE;
    v_line_row   vw_seismic_2d%ROWTYPE;
BEGIN

    FOR v_line_row IN select * from vw_seismic_2d order by gid LOOP

        FOR v_shotpt in select * from spt2(v_line_row.gid, v_interval, v_line_row.the_geom) LOOP
            return next v_shotpt;
        END LOOP;

    END LOOP;
    return;
END;
'
language 'plpgsql';
-- #########################################################################
CREATE OR REPLACE
    FUNCTION ComputeLabelAngle(integer, integer, geometry)
    RETURNS integer AS '
DECLARE
    v_last       ALIAS FOR $1;
    v_current    ALIAS FOR $2;
    v_geom       ALIAS FOR $3;
    v_numpts     integer;
    v_pt1        geometry;
    v_pt2        geometry;
    v_idx2       integer;
    v_rad        double precision;
    v_ang        integer;

BEGIN
    v_numpts = NumPoints(v_geom);

    if v_last < 0 then    -- Need to look ahead for 2nd point
        v_pt1 = PointN(v_geom, 1);
        v_idx2 = 10;
        if v_idx2 > v_numpts then
            v_idx2 = v_numpts;
        end if;
        v_pt2 = PointN(v_geom, v_idx2);
    else
        v_pt1 = PointN(v_geom, v_last+1);
        v_pt2 = PointN(v_geom, v_current+1);
    end if;

    -- Compute the angle using arctan
    -- v_ang = cast(degrees(atan2( x(v_pt2) - x(v_pt1), y(v_pt2) - y(v_pt1) ))  as integer);
    v_rad = atan2( x(v_pt2) - x(v_pt1), y(v_pt2) - y(v_pt1) );
    if v_rad < -pi()/2 then
        v_rad = v_rad + 3*pi()/2;
    elseif v_rad < 0.0 then
        v_rad = v_rad + pi()/2;
    elseif v_rad < pi()/2 then
        v_rad = v_rad + pi()/2;
    else
        v_rad = v_rad - pi()/2;
    end if;
    v_rad = pi()/2 - v_rad;   --  label rotation is from horizontal (pi/2)
 
    v_ang = cast(degrees(v_rad) as integer);
    return v_ang;
END;
'
language 'plpgsql';
-- #########################################################################
