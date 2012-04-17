--
--   $Id: gr_users.sql,v 1.1.1.1 2007/04/24 02:04:25 rob Exp $
--   $Name:  $
-- =============================================================
-- To replace gr_tables.sql
--
--
-- ====  Developer groups    ====
-- grant select, insert, update, delete on web_users to group devel;
grant all privileges on database geotest to group devel;
grant all privileges on language plpgsql to group devel;


-- ====  Data admin groups   ====

--       oss
grant select, insert, update, delete on client, client_users, client_userviews to group oss;
grant select, update, delete on vw_seismic_2d to group oss;

--       dfd
grant select, insert, update, delete on client, client_users, client_userviews to group dfd;
grant select, update, delete on vw_seismic_2d to group dfd;


-- ====  Public group        ====

--       pub
grant select on
            basins_noga,
            client_users,
            client_userviews,
            counties_100k,
            fips_counties,
            fips_states,
            geometry_columns,
            hydrogl,
            hydrogp,
            plays_noga,
            plss_100k,
            regions_noga,
            seds,
            spatial_ref_sys,
            states_100k,
            vw_seismic_2d,
            vw_seismic_2d_img
    to group pub;

grant select on vw_seismic_2d to group pub;
grant select on vw_seismic_2d_img to group pub;


-- group www
grant select, insert, update, delete on web_users to group www;
