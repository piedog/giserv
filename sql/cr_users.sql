-- ###############################################################################
-- $Id: cr_users.sql,v 1.1.1.1 2007/04/24 02:04:25 rob Exp $
-- $Name:  $
-- ###############################################################################
--
-- Create groups
--
-- ==========================================================================
create group devel;     -- Developer user group
create user rob with password 'rob' in group devel;

-- ==========================================================================
-- Group Optimum Seismic Services (oss)
create group oss;       -- Optimum Seismic Services
create user steves with password 'steves' in group oss;
create user samr with password 'samr' in group oss;

-- ==========================================================================
-- Group DataFlow Design (dfd)
create group dfd;       -- DataFlow Design
create user robp with password 'robp' in group dfd;
create user susanc with password 'susanc' in group dfd;

-- ==========================================================================
-- Group public users   Everyone needs to be in this group except www
create group pub;       -- Public users, can access any non-sensitive data
create user guest with password 'guest' in group pub;

-- Also, everyone needs to be in this group except www and devel
alter group pub add user robp;
alter group pub add user susanc;
alter group pub add user steves;
alter group pub add user samr;


-- ==========================================================================
create group www;       -- Internet login only to read/write to session table.
create user nobody with password 'nobody' in group www;
