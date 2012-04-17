-------------------------------------------------------------------------------
--   $Id: cr_tables.sql,v 1.1.1.1 2007/04/24 02:04:25 rob Exp $
--   $Name:  $
-------------------------------------------------------------------------------
--  For database creation, see postgis install notes:  README.postgis
--
drop table client_userviews;
drop table client_users;
drop table client;
create table client (
    clt_id    varchar(8),  -- PK
    clt_name  varchar(64),
    primary key (clt_id)
);


create table client_users (
    cusr_username   varchar(64),  -- PK
    clt_id          varchar(8),   -- FK
    cusr_last_view  varchar(32),
    primary key (cusr_username),
    foreign key (clt_id) references client (clt_id)
    on delete cascade
);
--
--
create table client_userviews (
    cusr_username      varchar(64),  -- PK
    usrv_viewname      varchar(32),  -- PK
    usrv_active_layer  varchar(12),
    usrv_proj_str      varchar(1024),
    usrv_extents_str   varchar(1024),
    usrv_layer_str     varchar(1024),
    primary key (cusr_username, usrv_viewname),
    foreign key (cusr_username) references client_users (cusr_username)
    on delete cascade
);

-- done: alter table client_userviews add column usrv_active_layer varchar(12);
--
-- Remove the clt_id as index and make a foreign key
--alter table client_users drop constraint client_users_pkey cascade;
--alter table client_userviews drop constraint client_userviews_pkey cascade;

--alter table client_users add constraint client_users_pkey unique (cusr_username);
--alter table client_userviews add constraint client_userviews_pkey unique (cusr_username,usrv_viewname);
--alter table client_users add constraint 

create table web_users (
    session_id         char      (32)   not null,
    password           varchar   (32)   not null default '',
    ip                 varchar   (15)   not null default '',
    exp_date           timestamp,
  primary key (session_id)
);
