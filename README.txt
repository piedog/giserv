About:
Author: rgp@geodatadesign.com

Giserv is a web application developed as a web mapping and inventory tool for managing, displaying, and
querying seismic data from a PostgreSQL database. It was designed for a seismic data service company to allow its client companies
to log into the system to view, manage, and edit their holdings.

The service company manages digital data, most of which are on tape and some on hard drives. They also manage hard-copy information
that is in the form of printed maps, printed seismic sections, and hand-written survey notes. One of the purposes of the database
is to provide links between the geographic seismic survey and the hardcopy data.

Security was designed to segregate the data by client company, so that users from  one company cannot access the data from another
company. Individual users are assigned accounts and can be managed by a client company administrator.


It was originally written in php4 in 2004 and upgraded for php5 in 2006. Giserv uses the Mapserver php scripting engine and PostgreSQL
with the PostGIS database extensions for performing geographic queries.

Links:
http://www.mapserver.org/about.html
http://www.postgis.org
http://www.postgresql.org
