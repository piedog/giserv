<?php
### ==========================================================================
###   $Id: mapmgr.inc,v 1.1.1.1 2007/04/24 02:04:24 rob Exp $
###   $Name:  $
### ==========================================================================
    require_once 'DB.php';
    require_once 'userlogin.inc';

    ### ======================================================================

    function mapToLayerstring(& $mapobj) {
        ## Encode information  for all layers (visible/invisible, etc)
        ##    into single string
        ## The encoded string is database store format for layers

        $str = "";
        for ($i=0; $i<$mapobj->numlayers; $i++) {
            $layer = $mapobj->getLayer($i);
            if ($str != "") $str .= ";";
            ## encode visibility status and dummy annotation status
            $str .= $layer->name . "=" . $layer->status . ",0";
        }
        return $str;
    }

    function layerstringToMap($str, &$mapobj) {
        ## Decode the layer list from the layer string
        ##        (grid=1,0;samples=0,0,;...;seis=1,1)
        ## After splitting the string, we will have a list of strings of the form:
        ##      samples=1,0
        $list = split(';', $str);    ## column is a ';' separated list of layers
        ##echo "str= " . $str . "<br />\n";
        for ($i=0; $i<count($list); $i++) {
            $layerStr = $list[$i];
            $visibility = 0;
            $labelling = 0;

            list($name, $restOfStr) = split('=',$layerStr);
            $strings = split(',', $restOfStr);  ## $restOfStr contains "," separated values
                                                ## This version only has 2 values
            if (count($strings) > 0) $visibility = $strings[0];
            if (count($strings) > 1) $labelling = $strings[1];
            ##echo "name=" . $name . ",restOfStr= " . $restOfStr. ",layerStr=".$layerStr . "<br />\n";

            # Update the map object with the parsed values. If there is no
            # layer with name of $name, ignore
            if (  @$mapobj->getLayerByName($name) ) {
                $layerObj = $mapobj->getLayerByName($name);
                if ($layerObj->getMetaData("layertype") != "hightlight") {
                    $layerObj->set( 'status', ($visibility ? MS_ON : MS_OFF) );
#echo "layerstringToMap(): name=" . $layerObj->name . ", visibility: " . $visibility . "  " .  $layerObj->status . "<br />\n";
                }
           }
        }

    }


    function findIndex($str, &$list) {
        # Return the index of the string $str in list
        ## list must be a associative array
        $j = 0;
        foreach ($list as $i => $u) {
            if ($list[$i] == $str) return $j;
            $j++;
        }

        ## If not in list of views, return false
        if ( $i == count($list) ) return -1;
    }


    function mapToLayerPanel(&$mapObj, &$wk) {
        ## Update the widget key pairs array using layer info from the map object
        for ($i=0; $i<$mapObj->numlayers; $i++) {
            $layerObj = $mapObj->getLayer($i);
            if ($layerObj->getMetaData("layertype") != "highlight") {
                $visibility = $layerObj->status == MS_ON || $layerObj->status == MS_DEFAULT ? "Y" : "";
                $wk[$layerObj->name . '_vi']->setValue($visibility);
#echo "mapToLayerPanel(): name=" . $layerObj->name . ", vis=" . $visibility . "<br />\n";
            }
        }

    }

    ### ======================================================================

    class MapManager {

        var $mapObj = null;
        var $dbH = null;         ## database connection handle
        var $curViewName = null;
        var $viewNameList = array();
        var $lastView = "";      ## Name of view from previous session
                                 ## Eventually use this to load when user enters session
        var $activeLayer = null;
        var $origExtent = null;
        var $origProj = null;


        function MapManager ($db, $mapfile, &$dbUconn, $vname=null) {
            $this->dbH = $db;
            $this->mapObj = ms_newMapObj($mapfile);   ## load map parameters from file

            # To avoid hardcoding the document path, use server variable and url
            $path = $_SERVER["DOCUMENT_ROOT"] . $this->mapObj->web->imageurl;
            $this->mapObj->web->set("imagepath", $path);


            $this->curViewName = $vname;

            $this->origExtent = ms_newRectObj();
            $this->origExtent->setextent($this->mapObj->extent->minx, $this->mapObj->extent->miny,
                                         $this->mapObj->extent->maxx, $this->mapObj->extent->maxy);
            $this->origProj = $this->mapObj->getProjection();
            

            # load names of all views for this user ----------------
            $sql = "select usrv_viewname from client_userviews where cusr_username = USER order by usrv_viewname";
            $result = $this->dbH->query($sql);
            if (DB::iserror($result))   die ($result->getMessage());

            while ($row = $result->fetchRow()) {
                if (DB::isError($row))  die ($row->getMessage());
                #array_push($this->viewNameList, $row['usrv_viewname']);
                $this->viewNameList[$row['usrv_viewname']] = $row['usrv_viewname'];
            }
            $result->free();

            ## get the last view from the client_users table ----------------
            ## Note that this sql will select the last view only if it is in the client_userviews table.
            ## Otherwise it will return an empty row.
            $sql =  'select usrv_viewname from client_userviews v, client_users u'
                 . ' where u.cusr_username = v.cusr_username'
                 . ' and u.cusr_last_view = v.usrv_viewname and u.cusr_username = USER';
            $v = $this->dbH->getOne($sql);
            if (DB::isError($v)) die ($v->getMessage());

            $this->lastView = $v ? $v : ( count($this->viewNameList) ?  key($this->viewNameList) : null );

            ## Reset the connect string for each PostGresql layer
            ## to current user'
            ## For postgresql layer types, reset the connect to the current
            ## username and password
            for ($i=0; $i<$this->mapObj->numlayers; $i++) {
                $layer = $this->mapObj->getLayer($i);
                ##echo $layer->name . ", status=" . $layer->status . "<br />\n";
                if ($layer->connectiontype == MS_POSTGIS) {

		    # check for substitution
                    if ($layer->getMetaData("subs_mapsrid") == "1") {
	                ## -- The formula: int srid = 33001 + (180+(int)lon_0)/5 *100 + (75+(int)lat_0)/5;
			## -118.11870732 49.5300602652 34265 
		        $xyc = $this->getLLCenter();
			$newSrid = 33001 + (int)((180 + $xyc->x)/5) * 100 + (int)((75 + $xyc->y)/5);
		        $newData = preg_replace('/subs_mapsrid/', $newSrid, $layer->data);
                        $layer->set('data', $newData);
                        ##echo $xyc->x ." ".$xyc->y." ".$newSrid;
		    }
                      $layer->set("connection", "user=" . $dbUconn->getUsername()
                                  . " password=" . $dbUconn->getPassword()
                                  . " dbname=" . $dbUconn->getDBName()
                                  . " host=" . $dbUconn->getDBHost()
                                 );
		}
            }
        }

        function load($vname) {     ## reset map parameters from database
            $this->name = $vname;
            # Retrieve data for named view"
            $sql = "select * from client_userviews where cusr_username=USER"
                  . " and usrv_viewname='" . $vname . "' order by usrv_viewname";

            $result = $this->dbH->query($sql);
            if (DB::iserror($result))   die ($result->getMessage());

            $row = $result->fetchRow();
            if (DB::isError($row))  die ($row->getMessage());

            layerstringToMap($row['usrv_layer_str'], $this->mapObj);

            ## Update the projection string, extents, and active layer
            ##  in map file from database info
            if (isset($row['usrv_active_layer']))  $this->activeLayer = $row['usrv_active_layer'];
            if (isset($row['usrv_proj_str']))   $this->mapObj->setProjection($row['usrv_proj_str']);
            if (isset($row['usrv_extents_str'])) {
                list($minx,$miny,$maxx,$maxy) = split( ' ', $row['usrv_extents_str'] );
                $this->mapObj->setextent($minx,$miny, $maxx, $maxy);
            }
            $result->free();

            $this->curViewName = $vname;
        }


        function setFromRequest( & $rq_array) {   ## Reset map parameters from request variables
            ## request variables passed from previous screen are
            ## named using the following convention:
            ##   NAME_ATTRIBUTE or {grid,state,cnty,trng,lake,seis,samp}_{vi,lb}

            ## Set layers inside the map object according to the
            ## layer defs that were posted
            for ($i=0; $i<$this->mapObj->numlayers; $i++) {
                $layerObj = $this->mapObj->getLayer($i);

                $varName = $layerObj->name . "_vi";   ## name of post var for visibility status
                if ( isset($rq_array[$varName])  && $rq_array[$varName] == 'Y')
                    $layerObj->set( "status", 1);
                else
                    $layerObj->set( "status", 0);

                $varName = $layerObj->name . "_lb";   ## name of post var for labelling status
                if ( !isset($rq_array[$varName])  || $rq_array[$varName] != 'Y') {
                    #$class = &$layerObj->getClass(0);
                    #$class->labelObj = null; 
                    $layerObj->set("labelminscale", 0);
                    $layerObj->set("labelmaxscale", 0);
                }
            }

            if (isset($rq_array['projstr'])) $this->mapObj->setProjection($rq_array['projstr']);
            if (isset($rq_array['extents'])) {
                list($minx,$miny,$maxx,$maxy) = split( ' ', $rq_array['extents'] );
                $this->mapObj->setextent($minx,$miny,$maxx,$maxy);
            }
            if (isset($rq_array['actLayer'])) $this->activeLayer = $rq_array['actLayer'];
        }


        #function debugPrint() {
        #    echo "<!--  View::debugPrint(), " . $this->name . "    -->\n";
        #    foreach ($this->layerInfoList as $i => $u)
        #        $this->layerInfoList[$i]->debugPrint();
        #}


        function getActiveLayer() { return $this->activeLayer; }

        function setActiveLayer($layerName) {
            ## Returns false if layer name does not exist
            $this->activeLayer = $layerName;
        }


        function saveAs($vname) { ## Save current map settings to database under new view name

            #if ( ($idxV = findIndex($vname, $this->viewNameList)) > -1)
            if (array_key_exists($vname, $this->viewNameList))
                return false; ## View name already exists

            $sql = "select count(*) as nrows from client_userviews"
                  . " where cusr_username=USER and usrv_viewname='" . $vname . "'";

            $result = $this->dbH->getOne($sql);
            if (DB::iserror($result))   die ($result->getMessage());
            if ($result['nrows'] != 0) return false;    ## return if view already exists


            $layerStr = mapToLayerstring($this->mapObj);
            $projStr  = $this->mapObj->getProjection();
            $extent   = $this->mapObj->extent;
            $extentStr = $extent->minx . " " . $extent->miny . " " . $extent->maxx . " " . $extent->maxy;
            $sql = "insert into client_userviews"
                . " (cusr_username,usrv_viewname,usrv_proj_str,"
                . "usrv_extents_str,usrv_layer_str,usrv_active_layer)"
                . " values (USER,'" . $vname . "','" . $projStr . "'"
                . ",'" . $extentStr . "','" . $layerStr . "','" . $this->activeLayer . "')";
            $result = $this->dbH->query($sql);
            if (DB::iserror($result))   die ($result->getMessage());
            $this->dbH->commit();

            #array_push($this->viewNameList, $vname);
            $this->viewNameList[$vname] = $vname;
            asort($this->viewNameList);

            $this->curViewName = $vname;
            return true;
        }


        function save($vname) { ## Update current map settings to database under existing view name

            #if ( ($idxV = findIndex($vname, $this->viewNameList)) < 0)
            if (!array_key_exists($vname, $this->viewNameList))
                return false; ## view name not in list

            $sql = "select count(*) as nrows from client_userviews"
                  . " where cusr_username=USER and usrv_viewname='" . $vname . "'";

            $result = $this->dbH->getOne($sql);
            if (DB::iserror($result))   die ($result->getMessage());
            if ($result['nrows'] != 1) return false;    ## return if view already exists

            $layerStr = mapToLayerstring($this->mapObj);
            $projStr  = $this->mapObj->getProjection();
            $extent   = $this->mapObj->extent;
            $extentStr = $extent->minx . " " . $extent->miny . " " . $extent->maxx . " " . $extent->maxy;

            $sql = "update client_userviews"
                 . " set usrv_proj_str='" . $projStr
                 . "',usrv_extents_str='" . $extentStr
                 . "',usrv_layer_str='" . $layerStr
                 . "',usrv_active_layer='" .  $this->activeLayer  . "'"
                 . " where cusr_username=USER and usrv_viewname='" . $vname . "'";


            $result = $this->dbH->query($sql);
            if (DB::iserror($result))   die ($result->getMessage());
            $this->dbH->commit();

            $this->curViewName = $vname;
            return true;
        }


        function delete($vname) { ## Delete existing view from database

            #if ( ($idxV = findIndex($vname, $this->viewNameList)) < 0) return false;
            if (!array_key_exists($vname, $this->viewNameList)) return false;

            $sql = "delete from client_userviews"
                 . " where cusr_username=USER and usrv_viewname='" . $vname . "'";
            $result = $this->dbH->query($sql);
            if (DB::iserror($result))   die ($result->getMessage());
            $this->dbH->commit();

            #array_splice($this->viewNameList, $idxV, 1);  ## Remove the view name from the list
            $idxV = findIndex($vname, $this->viewNameList);
            array_splice($this->viewNameList, $idxV, 2);  ## Remove the key and view name from the list

            if ($vname == $this->curViewName)
                $this->curViewName =  null;
            return true;
        }


        function & getViewNameList()  { return $this->viewNameList; }
        function & getMapObject() { return $this->mapObj; }
        function getViewName() { return $this->curViewName; }

        function getOrigExtent() {
            return $this->origExtent;
        }

        function getOrigProj()   { return $this->origProj; }

        function getXYCenter() {

            ##get center xy point
            $xc = ($this->mapObj->extent->maxx + $this->mapObj->extent->minx)/2;
            $yc = ($this->mapObj->extent->maxy + $this->mapObj->extent->miny)/2;
            $ptc = ms_newPointObj();
            $ptc->setXY($xc, $yc);
            return $ptc;
        }

        function getLLCenter() {

            ##get center lat-long point
            $xc = ($this->mapObj->extent->maxx + $this->mapObj->extent->minx)/2;
            $yc = ($this->mapObj->extent->maxy + $this->mapObj->extent->miny)/2;
            $ptc = ms_newPointObj();
            $ptc->setXY($xc, $yc);

            $prjIn = ms_newProjectionObj($this->mapObj->getProjection());
            $prjOut = ms_newProjectionObj("proj=latlong");
            $ptc->project($prjIn, $prjOut);
            return $ptc;
        }

        function getLLExtent() {
            $ptList = array();
            for ($i=0; $i<4; $i++)
                $ptList[$i] = ms_newPointObj();

            $rtn = $ptList[intval(0)]->setXY(doubleval($this->mapObj->extent->minx), doubleval($this->mapObj->extent->miny));
            $rtn = $ptList[intval(1)]->setXY(doubleval($this->mapObj->extent->minx), doubleval($this->mapObj->extent->maxy));
            $rtn = $ptList[intval(2)]->setXY(doubleval($this->mapObj->extent->maxx), doubleval($this->mapObj->extent->maxy));
            $rtn = $ptList[intval(3)]->setXY(doubleval($this->mapObj->extent->maxx), doubleval($this->mapObj->extent->miny));

            $prjIn = ms_newProjectionObj($this->mapObj->getProjection());
            $prjOut = ms_newProjectionObj("proj=latlong");

            for ($i=0; $i<count($ptList); $i++)
                $ptList[$i]->project($prjIn, $prjOut);

            $lonmin = $ptList[0]->x; $lonmax = $ptList[0]->x;
            $latmin = $ptList[0]->y; $latmax = $ptList[0]->y;
            for ($i=0; $i<count($ptList); $i++) {
                $lonmin = $lonmin < $ptList[$i]->x ? $lonmin : $ptList[$i]->x;
                $lonmax = $lonmax > $ptList[$i]->x ? $lonmax : $ptList[$i]->x;
                $latmin = $latmin < $ptList[$i]->y ? $latmin : $ptList[$i]->y;
                $latmax = $latmax > $ptList[$i]->y ? $latmax : $ptList[$i]->y;
            }
            for ($i=0; $i<count($ptList); $i++)
                $ptList[$i]->free();
            $ext = ms_newRectObj();
            $ext->setextent($lonmin, $latmin, $lonmax, $latmax);
            return $ext;
        }
    }


    ### ======================================================================
?>
