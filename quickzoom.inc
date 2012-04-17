<?php
############################################################################################
##  $Id: quickzoom.inc,v 1.1.1.1 2007/04/24 02:04:19 rob Exp $
##  $Name:  $
############################################################################################
    function getStateExtent(&$db, $state) {

        ## Zoom to the requested latlong point and center map around it  

        $extent = ms_newRectObj();
        if ( strtoupper($state) == 'AK' ) {  ## Alaska a special case due to date line
            $sql = "select mnx,mxx,mny,mxy from"
                        . " (select min(v) as mnx from ("
                        . " select min(xmin(the_geom))-360 as v"
                        . " from states_100k where state_fips='02'"
                        . " and xmax(the_geom) > 0 group by state_fips"
                        . " union"
                        . " select min(xmin(the_geom)) as v"
                        . " from states_100k where state_fips='02'"
                        . " and xmax(the_geom) < 0 group by state_fips ) r ) rx1,"
                        . " (select max(xmax(the_geom)) as mxx"
                        . " from states_100k where state_fips='02'"
                        . " and xmax(the_geom) < 0 group by state_fips) rx2,"
                        . " (select min(ymin(the_geom)) as mny, max(ymax(the_geom)) as mxy"
                        . " from states_100k where state_fips='02' group by state_fips ) ry"
                    ;

            $db->query($sql);
            if (DB::isError($db)) echo $db->getMessage() . "\n";
            $q = $db->query($sql);
            if (DB::isError($q)) echo $db->getMessage() . "\n";
            $row = $q->fetchRow(DB_FETCHMODE_ASSOC);
            $extent->setextent( $row['mnx'], $row['mny'], $row['mxx'], $row['mxy'] );
            $q->free();
        }
        else {                               #######################  Not alaska
            $sql = "select extent(the_geom) as geom"
                    . " from states_100k s, fips_states f"
                    . " where s.state_fips = f.state_n_code"
                    . " and f.state_a_code = upper('" . $state . "')";

            ##$db->query($sql);
            ##if (DB::isError($db)) echo $db->getMessage() . "\n";
            $q = $db->query($sql);
            if (DB::isError($q)) echo $db->getMessage() . "\n";

            ## postgis-0.8.1       : BOX3D(-88.47202 30.189623 0,-84.89348 35.008881 0)
            ## postgis-1.0.0-rc3   : BOX(-88.472 30.1896,-84.8935 35.0089)
            $row = $q->fetchRow(DB_FETCHMODE_ASSOC);

            $tmp = preg_replace('/[a-zA-Z0-9]+\((.+)\)/', '$1', $row['geom']);  ## return string in (  )
            $pts = split(" *, *", $tmp);  # return array of points: xx.xxx,yy.yyy (we should have 2 poin pairs)
            list($minx,$miny) = split(' +', $pts[0], 2); ## only pick off 2 numbers. If BOX3D, then there are 3 
            list($maxx,$maxy) = split(' +', $pts[1], 2);
            $minx = $minx - 0.05 * ($maxx-$minx);
            $maxx = $maxx + 0.05 * ($maxx-$minx);
            $miny = $miny - 0.05 * ($maxy-$miny);
            $maxy = $maxy + 0.05 * ($maxy-$miny);
            $extent->setextent( $minx,$miny,$maxx,$maxy );
            $q->free();
        }
        return $extent;
    }


    function getCountyExtent(&$db, $cntycode) {
        ## Query county polygons to get geometry extents for requested county

            $sql = "select extent(the_geom) as geom"
                    . " from counties_100k where fips='" . $cntycode . "'";
        ##$ext = getExtentFromSQL($db, $sql);
        if (!($ext = getExtentFromSQL($db, $sql))) return $ext;
        $minx = $ext->minx - 0.05 * ($ext->maxx-$ext->minx);
        $maxx = $ext->maxx + 0.05 * ($ext->maxx-$ext->minx);
        $miny = $ext->miny - 0.05 * ($ext->maxy-$ext->miny);
        $maxy = $ext->maxy + 0.05 * ($ext->maxy-$ext->miny);
        $ext->setExtent(  $minx, $miny, $maxx, $maxy );
        return $ext;
    }


    function getRegionExtent(&$db, $regNum) {

        $sql = "select extent(the_geom) as geom"
                . " from regions_noga"
                . " where reg_num = " . $regNum;

        return getExtentFromSQL($db, $sql);
    }


    function getTwnrngExtent(&$db, $mer_code, $twnnum, $twndir, $rngnum, $rngdir) {
        #$sql = "select extent(the_geom) as geom from plss_100k"
        $sql = "select box3d(the_geom) as geom from plss_100k"
             . " where meridian_code='" . $mer_code . "'"
             . " and twn_num=" . $twnnum . " and rng_num=" . $rngnum
             . " and twn_dir='" . $twndir . "' and rng_dir='" . $rngdir . "'";
        if (!($ext = getExtentFromSQL($db, $sql))) return $ext;
        $minx = $ext->minx - 0.25 * ($ext->maxx-$ext->minx);
        $maxx = $ext->maxx + 0.25 * ($ext->maxx-$ext->minx);
        $miny = $ext->miny - 0.25 * ($ext->maxy-$ext->miny);
        $maxy = $ext->maxy + 0.25 * ($ext->maxy-$ext->miny);
        $ext->setExtent(  $minx, $miny, $maxx, $maxy );
        return $ext;
    }


    function getExtentFromSQL(&$db, $sql) {
        ##$db->query($sql);
        ##if (DB::isError($db)) echo $db->getMessage() . "\n";
        $q = $db->query($sql);
        if (DB::isError($q)) echo $db->getMessage() . "\n";

        if ($q->numRows() == 0) return null;

        $row = $q->fetchRow(DB_FETCHMODE_ASSOC);
        if ( !isset($row) || !isset($row['geom']) || preg_match('/^ *$/',$row['geom']) ) return null;

        $tmp = preg_replace('/[a-zA-Z0-9]+\((.+)\)/', '$1', $row['geom']);  ## return string in (  )
        $pts = split(" *, *", $tmp);  # return array of points: xx.xxx,yy.yyy (we should have 2 poin pairs)
        list($minx,$miny) = split(' +', $pts[0], 2); ## only pick off 2 numbers. If BOX3D, then there are 3 
        list($maxx,$maxy) = split(' +', $pts[1], 2);

        $extent = ms_newRectObj();
        $extent->setextent( $minx,$miny,$maxx,$maxy );
        $q->free();
        return $extent;
    }

    function zoomToExtent(&$map, $extent) {
        ## Given the extent rectangle and map object,
        ## setup the map object in order to zoom to the given extent.
        $lon0 = ($extent->minx + $extent->maxx)/2;
        $lat0 = $extent->miny;
        ### Rule of thumb: std parallels at 1/6 and 5/6 of latitude range
        $lat1 = $extent->miny + 5*($extent->maxy-$extent->miny)/6;
        $lat2 = $extent->miny + ($extent->maxy-$extent->miny)/6;

        $projstr = "proj=lcc,ellps=GRS80,lon_0=" . $lon0 . ",lat_0=" . $lat0
                . ",lat_1=" . $lat1 . ",lat_2=" . $lat2;
        $projIn = ms_newProjectionObj("proj=latlong");
        $projOut = ms_newProjectionObj($projstr);

        ## Project the extent region and fit to map height and width
        $extent->project($projIn, $projOut);
        $extent->fit($map->width, $map->height);

        $map->setProjection($projstr, MS_FALSE);
        $map->extent->setExtent( $extent->minx, $extent->miny, $extent->maxx, $extent->maxy);
    }
?>
