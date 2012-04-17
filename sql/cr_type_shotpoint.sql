--------------------------------------------------------------
-- $Id: cr_type_shotpoint.sql,v 1.1.1.1 2007/04/24 02:04:25 rob Exp $
-- $Name:  $
--------------------------------------------------------------
CREATE TYPE shotpoint AS
 (
          gid            integer,
          sequence_num   integer,
          station_num    varchar,
          elevation      double precision,
          angle          integer,
          indicator      char,
          the_geom       geometry
 );
--    Consider adding the following:
--             ind_position char { A=end point, B=bend point }
--             elevation double precision
--             oid (build using gid*10000 + sequence_num)
