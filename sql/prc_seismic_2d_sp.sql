--------------------------------------------------------------
-- $Id: prc_seismic_2d_sp.sql,v 1.1.1.1 2007/04/24 02:04:25 rob Exp $
-- $Name:  $
--------------------------------------------------------------
-- Usage   :       select sel_seismic_sp(<gid>);
-- Example :       select sel_seismic_sp(14);
--------------------------------------------------------------
-- CREATE TYPE shotpoint AS
-- (gid integer, sequence_num integer, station_num varchar, angle integer,the_geom geometry);

CREATE OR REPLACE
    FUNCTION sel_seismic_sp(integer)
    RETURNS SETOF shotpoint AS '
DECLARE
    v_gid        ALIAS FOR $1;
    i            integer;
    v_sta1     seismic_2d_sp.station_num%TYPE;
    v_sta2     seismic_2d_sp.station_num%TYPE;
    v_seq1     seismic_2d_sp.sequence_num%TYPE;
    v_seq2     seismic_2d_sp.sequence_num%TYPE;
    v_sta_rec  RECORD;
    v_geom     seismic_2d.the_geom%TYPE;
    v_pt       seismic_2d.the_geom%TYPE;
    v_row      shotpoint%ROWTYPE;


BEGIN

    -- Get first and last shotpoint numbers fro seismic_2d_sp table
    select sequence_num,station_num into v_seq1,v_sta1 from seismic_2d_sp s where s.gid = v_gid
           and s.sequence_num=(select min(sequence_num) from seismic_2d_sp where s.gid=gid);
    select sequence_num,station_num into v_seq1,v_sta2 from seismic_2d_sp s where s.gid = v_gid
           and s.sequence_num=(select max(sequence_num) from seismic_2d_sp where s.gid=gid);

    select the_geom into v_geom from seismic_2d where gid=v_gid;

    -- Get decimated sequence of stations
    FOR v_sta_rec in select gid,sequence_num,cast(station_num as numeric) as snum
        from seismic_2d_sp
        where gid=v_gid ORDER BY sequence_num LOOP

        if mod(v_sta_rec.snum,10) = 0 then
            v_row.gid           = v_gid;
            v_row.sequence_num  = v_sta_rec.sequence_num;
            v_row.station_num   = cast (v_sta_rec.snum as varchar);
            v_row.angle         = NULL;
            v_row.the_geom      = PointN(v_geom, v_sta_rec.sequence_num);
            
            --RAISE NOTICE ''  %  %  %'', v_sta_rec.sequence_num, v_sta_rec.snum, v_pt;
            return next v_row;
        end if;
    END LOOP;
    return;
END;
'
language 'plpgsql';
