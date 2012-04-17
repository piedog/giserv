<?php
###############################################################################
#
#  $Id: mapcmds.inc,v 1.1.1.1 2007/04/24 02:04:24 rob Exp $
#  $Name:  $
#
###############################################################################
    ##         Actions                Variables
    ##
    ##         sub_vw              set or not set   sub_vw {Save As, Save, Load, Delete}
    ##                             viewtoggle { rad_view_new, rad_view_upd, rad_view_lod, rad_view_del }
    ##         sub_fullext   
    ##         sub_logout   
    ##         sub_qz              sel_qz, txt_qzlat,  txt_qzlon, txt_qzlineid, sel_qzstate, sel_qzregion
    ##         sel_qz              q_lineid, q_yearshot, q_comp
    ##         pzgroup             pzgroup {rb_zin,rb_zou,rb_pan,rb_idn} 
    ##                                        extents, mainmap_x, mainmap_y, projstr, imagewidth, imageheight

    ## ========================================================================

    require_once 'quickzoom.inc';


    function wildcardToSQL($str) {
        $str = preg_replace('/_/','\\\\_', $str);
        $str = preg_replace('/\*/','%', $str);
        $str = preg_replace('/\?/','_', $str);
        return $str;
    }


    class MouseAction {

        var    $lookUpTbl;   # Attribute - value pairs
        var    $action;
        var    $isError = false;
        var    $errorMsg = "";
            
        function MouseAction() {  }
      
        function setAction($act) { $this->action = $act; }
   
        function getAction() { return $this->action; }
      
        function setValue ($a, $v) { $this->lookUpTbl[$a] = $v; }   

        function getValue ($a) {
           if ( !array_key_exists($a, $this->lookUpTbl)) return "";
           return $this->lookUpTbl[$a];
        }

        function setErrorMsg($msg) { $this->errorMsg = $msg;  $this->isError = true; }
        function getError()        { return $this->isError; }
        function getErrorMsg()     { return $this->errorMsg; }
   }

    ## ========================================================================
    function sqlBuildQuery_cseis(&$wk) {
        $sql = "";

        if ( ($str=$wk['q_lineid']->getValue()) != "") {
            $str = wildcardToSQL($str);
            $sql .= "upper(line_id) like " . "'" . strtoupper($str) . "'";
        }

        if ($wk['q_yearshot']->getValue() != "")  {
            if ($sql != "") $sql .= " AND ";
            $sql .= "extract(year from survey_date)='" . $wk['q_yearshot']->getValue() . "'";
        }

        if ( ($str=$wk['q_comp']->getValue()) != "")  {
            $str = wildcardToSQL($str);
            if ($sql != "") $sql .= " AND ";
            $sql .= "upper(client_company) like '" . strtoupper($str) . "'";
        }
        return $sql;
    }
    ## ========================================================================
    function sqlBuildQuery_streams(&$wk) {
        $sql = "";

        if ( ($str=$wk['q_txt_nameriv']->getValue()) != "") {
            $str = wildcardToSQL($str);
            $sql .= "upper(name) like '" . strtoupper($str) . "'";
#$sql = "gid=58041";
#$sql = "gid=57747";
        }
#echo $sql;
        return $sql;
    }
    ## ========================================================================
    function sqlBuildQuery_samples(&$wk) {
        $sql = "";
        $min = "";
        $max = "";
        for ($i=0; $i<4; $i++) {    ## Maximum of four elements on form
            if ( ($elem=$wk['q_sel_elem' .$i]->getValue()) != "") {
                $min=$wk['q_txt_minelem' .$i]->getValue();
                $max=$wk['q_txt_maxelem' .$i]->getValue();
                          
                if ($sql != "" && ($min != "" || $max != "")) $sql .= " and ";
                if ($min != "" && $max != "") $sql .= $elem . ">=" . $min . " and " . $elem . "<=" . $max;
                else if ($min != "") $sql .= $elem . ">=" . $min;
                else if ($max != "") $sql .= $elem . "<=" . $max;
            }
        }
        return $sql;
    }
    ## ========================================================================

    function getMouseAction (&$argArray, &$wk, &$maction) {
        // input:   argArray     hash for _POST or _GET
        // in/out:  maction      array( action => array ( var1 => value1, var2 => value2, ..., varN => valueN ) )
        ##echo "getMouseAction() n=" . count($wk['activelayer']);
        ##echo "getMouseAction(),actLayer=  " . $wk['activelayer']->getName()  . "  value=" . $wk['activelayer']->getValue(). "<br />\n";

        ####    A submit button was pressed                      #########
        if (array_key_exists('sub_vw', $argArray)) {
            if (!array_key_exists('viewtoggle', $argArray)) {
                $maction->setErrorMsg('missing value for viewtoggle');
                return false;
            }
            
            elseif ($argArray['viewtoggle'] == 'rad_view_new') {
                if ($wk['txt_vw_new']->validate() ) {
                    $maction->setAction('SAVEVIEWAS');
                    $maction->setValue('viewname', $wk['txt_vw_new']->getValue());
                }
                else
                    $maction->setAction("ERROR");
            }
            elseif ($argArray['viewtoggle'] == 'rad_view_upd') {
                $maction->setAction('SAVEVIEW');
                $maction->setValue('viewname', $argArray['sel_view'] );
            }
            elseif ($argArray['viewtoggle'] == 'rad_view_lod') {
                $maction->setAction('LOADVIEW');
                $maction->setValue('viewname', $argArray['sel_view'] );
            }
            elseif ($argArray['viewtoggle'] == 'rad_view_del') {
                $maction->setAction('DELETEVIEW');
                $maction->setValue('viewname', $argArray['sel_view'] );
            }
            else echo "Error, no choice for viewtoggle<br />\n";
        }

        #### Test if quickzoom submit button was pressed                 ##########

        elseif ( array_key_exists('sub_qz', $argArray) ) {

            if (array_key_exists('sel_qz', $argArray) && $argArray['sel_qz'] == 'pnl_qz_ll') {
                if ( $wk['txt_qzlat']->validate() && $wk['txt_qzlon']->validate() ) {
                    $maction->setAction('LLZOOM');
                    $maction->setValue( 'lat', $wk['txt_qzlat']->getValue() );
                    $maction->setValue( 'lon', $wk['txt_qzlon']->getValue() );
                }
                else
                    $maction->setAction("ERROR");
            }

            #elseif (array_key_exists('sel_qz', $argArray) && $argArray['sel_qz'] == 'zbyLine') {
            #    #$maction->setAction('LINEZOOM');
            #    #$maction->setValue( '      ', $wk['txt_qzlineid']->getValue() );
            #}

            elseif (array_key_exists('sel_qz', $argArray) && $argArray['sel_qz'] == 'pnl_qz_st') {
                $maction->setAction('STATEZOOM');
                $maction->setValue( 'state', $wk['sel_qzstate']->getValue() );
                $maction->setValue( 'county',$wk['sel_qzcnty']->getValue() );
            }

            elseif (array_key_exists('sel_qz', $argArray) && $argArray['sel_qz'] == 'pnl_qz_rg') {
                $maction->setAction('REGZOOM');
                $maction->setValue( 'region', $wk['sel_qzregion']->getValue() );
            }

            elseif (array_key_exists('sel_qz', $argArray) && $argArray['sel_qz'] == 'pnl_qz_tr') {
                $maction->setAction('TRZOOM');
                $maction->setValue( 'meridian', $wk['sel_qzmer']->getValue() );
                $maction->setValue( 'twnnum', $wk['txt_qztwn']->getValue() );
                $maction->setValue( 'rngnum', $wk['txt_qzrng']->getValue() );
                $maction->setValue( 'twndir', $wk['ch_qztwn']->getValue() );
                $maction->setValue( 'rngdir', $wk['ch_qzrng']->getValue() );
            }
        }

        #### Test if Highlight Selection (Query by attribute was pressed ##########
        elseif (array_key_exists('sub_qy', $argArray) ) {
            if (isset($argArray['activelayer'])) {
                $maction->setValue("active_layer", $argArray['activelayer']);
            }
            else {
                #echo "active layer not selected<br />\n";
                #return;
                $maction->setValue("active_layer", "cseis");
            }

            $sql = "";
            $dummy = "'DUMMY'";  ## a place holder in the select list (select 'DUMMY', x, y from ...)
                                 ## Forgot why I did this. I think because the 2nd param in queryByAttributes
                                 ## is the sql where clause when using Pg and the first param is irrelevant

            if ($maction->getValue("active_layer") == "cseis") {
                $sql = sqlBuildQuery_cseis($wk);
            }
            else if ($maction->getValue("active_layer") == "streams") {
                $sql = sqlBuildQuery_streams($wk);
            }
            else if ($maction->getValue("active_layer") == "samples") {
                $sql = sqlBuildQuery_samples($wk);
            }
            $maction->setAction("QUERYATTR");
            $maction->setValue('sql', $sql);
            $maction->setValue('dummy', $dummy);
        }


        #### Test if reference map was clicked                           ##########
        elseif (array_key_exists('refmap_x',$argArray) && $argArray["refmap_x"]) {
            // The reference map image was clicked
            $maction->setAction("REFPOINT");
            $clickpt = ms_newPointObj();
            $clickpt->setXY( doubleval($argArray["refmap_x"]), doubleval($argArray["refmap_y"]));
            $maction->setValue("refpoint", $clickpt);
        }

        ## Reset to original extents  (Reset Extents button was clicked) ##########
        elseif (array_key_exists('sub_fullext',$argArray) ) {
                ##&& $argArray["mapcontrol"] == "Full Map") {
            $maction->setAction("RESET");
        }

        ## Logout and return to login screen (Logout button was clicked) ##########
        elseif (array_key_exists('sub_logout',$argArray) ) {
            $maction->setAction("LOGOUT");
        }

        #### Redraw map (refresh)  (Redraw button was clicked)           ##########
        elseif (array_key_exists('sub_fullext',$argArray)) {
                ######&& $argArray["sub_layer"] == "Redraw") {
            $maction->setAction("REDRAW");
        }


        #### Pan operations using the corner points                      ##########

        elseif (array_key_exists('pan_nw_x', $argArray) && $argArray["pan_nw_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan", "NW");
        }
        elseif ( array_key_exists('pan_n_x',$argArray) && $argArray["pan_n_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan","N");
        }
        elseif ( array_key_exists('pan_ne_x',$argArray) && $argArray["pan_ne_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan","NE");
        }
        elseif ( array_key_exists('pan_w_x',$argArray) && $argArray["pan_w_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan","W");
        }
        elseif ( array_key_exists('pan_e_x',$argArray) && $argArray["pan_e_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan","E");
        }
        elseif ( array_key_exists('pan_sw_x',$argArray) && $argArray["pan_sw_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan","SW");
        }
        elseif (array_key_exists('pan_s_x',$argArray) && $argArray["pan_s_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan","S");
        }
        elseif (array_key_exists('pan_se_x',$argArray) && $argArray["pan_se_x"]) {
            $maction->setAction("PAN");
            $maction->setValue("pan","SE");
        }

        #### Test for single click on map                                ##########

        else if (   ( array_key_exists('mainmap_x', $argArray) && $argArray['mainmap_x'] )
                 && ( ! array_key_exists('imgbox', $argArray)
                        || ! $argArray['imgbox'] || $argArray['imgbox'] == "" || $argArray['imgbox'] == "-1 -1 -1 -1")
                      ) {
                $maction->setAction("MAPPOINT");
                $clickpt = ms_newPointObj();
                $clickpt->setXY(doubleval( $argArray["mainmap_x"]), doubleval($argArray["mainmap_y"]) );
                $maction->setValue("mappoint", $clickpt);
                $maction->setValue("zoom_mode", $argArray['pzgroup']);
                ##echo "maction->setValue()=" .  $wk['activelayer']->getValue() . "\n";

                if (isset($argArray['activelayer'])) $maction->setValue("active_layer", $argArray['activelayer']);
                else $maction->setValue("active_layer", "cseis");
                $maction->setValue("zoom_fact", 2);
        }  ## end of mainmap_x

        #### Test for single click on map                                ##########
        else if (array_key_exists('imgxy', $argArray) && $argArray['imgxy']) {
                ##echo "here, " . $argArray['imgbox'] . ", " . $argArray['pzgroup'];
                $maction->setAction("MAPPOINT");
                list($xpixClick,$ypixClick) = explode(' ',$argArray["imgxy"]);
                $clickpt = ms_newPointObj();
                $clickpt->setXY( doubleval($xpixClick), doubleval($ypixClick));
                $maction->setValue("mappoint", $clickpt);
                $maction->setValue("zoom_mode", $argArray['pzgroup']);
                if (isset($argArray['activelayer'])) $maction->setValue("active_layer", $argArray['activelayer']);
                else $maction->setValue("active_layer", "cseis");
                $maction->setValue("zoom_fact", 2);
        }  ## end of imgxy

        ####  Test if rectangle was drawn                                ##########
        elseif (array_key_exists('imgbox',$argArray)
                && $argArray["imgbox"] != "-1 -1 -1 -1") {
            list ($lft,$top,$rgt,$bot) = explode(' ', $argArray["imgbox"]);
            ##echo "<!-- imgbox=" . $argArray["imgbox"] . "-->\n";

            if (isset($argArray['activelayer'])) $maction->setValue("active_layer", $argArray['activelayer']);
            else $maction->setValue("active_layer", "cseis");

            // Test for degenerate rectangle. If it is, average the extents and
            //      call it a point
            if (abs($rgt - $lft) < 5 || abs($top - $bot) < 5) {
                $clickpt = ms_newPointObj();
                $maction->setAction("MAPPOINT");
                $xpixClick = ($lft + $rgt)/2;
                $ypixClick = ($top + $bot)/2;
                $clickpt->setXY(doubleval($xpixClick), doubleval($ypixClick));
                $maction->setValue("mappoint", $clickpt);
                $maction->setValue("zoom_mode", $argArray['pzgroup']);
                $maction->setValue("zoom_fact", 2);
            }
            else {
                $zoomRect = ms_newRectObj();
                $zoomRect->setExtent ($lft, $top, $rgt, $bot);
                $maction->setAction("MAPRECT");
                $maction->setValue("maprect", $zoomRect);
                $maction->setValue("zoom_mode", $argArray['pzgroup']);
            }
        }   ## end of imgbox

        else
            return false;

        return true;
    }

   ## ========================================================================



    function execCommand(&$uconn, &$maction, &$wk, & $mapmgr,  & $querySet, &$errorMsgs) {

        $db = $uconn->getConnect();    ## Get db handle from user connection
        $map = & $mapmgr->getMapObject();
        $cur = $map->extent;

        ###  Handle actions from view manager           ###########

        if ($maction->getAction() == "SAVEVIEW") {
            $viewname = $maction->getValue("viewname");
            $mapmgr->save($viewname);
        }

        elseif ($maction->getAction() == "SAVEVIEWAS") {
            $viewname = $maction->getValue("viewname");
            $mapmgr->saveAs($viewname);
        }

        elseif ($maction->getAction() == "LOADVIEW") {
            $viewname = $maction->getValue("viewname");
            $mapmgr->load($viewname);
            if (isset($wk['activelayer'])) $maction->setValue("active_layer", $wk['activelayer']->getValue());
            else $maction->setValue("active_layer", "cseis");
            ####$wk['activelayer']->setValue($mapmgr->getActiveLayer());
            mapToLayerPanel($mapmgr->getMapObject(), $wk);
        }

        elseif ($maction->getAction() == "DELETEVIEW") {
            $viewname = $maction->getValue("viewname");
            $mapmgr->delete($viewname);
        }

        #### Redraw using same extents                ###########

        elseif ($maction->getAction() == "REDRAW") {
           ## Don't need to do anything here
        }


        #### Reset map using original extents         ###########

        elseif ($maction->getAction() == "RESET") {
            ## The button to draw the map with original extents was clicked
            $origext = $mapmgr->getOrigExtent();
            $origprj = $mapmgr->getOrigProj();
            $map->setExtent($origext->minx,$origext->miny,$origext->maxx,$origext->maxy);
            $map->setProjection( $origprj );
        }

        #### Logout, destroy sessionid, delete        ##########
          ## session from db

        elseif ($maction->getAction() == "LOGOUT") {
            $uconn->logout();   ## logoff database
            ## Get the extension name since this could be running as cgi (.phtml) or mod_php (.php)
            header('Location: giserv_login.'
                . preg_replace('/^.*\.(\w+)$/', '\1', $_SERVER['PHP_SELF'])
                . '?redir_error=' . $uconn->getUsername()
                . ' logged out');  ## redirect to login screen
        }

        ### Mouse click on map                        ##########

        elseif ($maction->getAction() == "MAPPOINT") {
            $tmppoint = $maction->getValue("mappoint");
            $pixClick = ms_newPointObj();
            $pixClick->setXY($tmppoint->x, $tmppoint->y);

            // World coordinates of clicked point
            $xwClick = $cur->minx + $tmppoint->x*($cur->maxx-$cur->minx)/$map->width;
            $ywClick = $cur->miny
                        + ($map->height-$tmppoint->y)*($cur->maxy-$cur->miny)/$map->height;

            $pixClick = ms_newPointObj();
            $pixClick->setXY($tmppoint->x, $tmppoint->y);

            if ($maction->getValue("zoom_mode") == "rb_zin") {          ## Zoom in

                ## ----------------------------------------------------------------------
                ## Optimize projection by reprojecting on the fly
                ## get lat/long of point so we can reproject
                $llClick = ms_newPointObj();
                $llClick->setXY($xwClick, $ywClick);

                ## 


                $prjIn = ms_newProjectionObj($map->getProjection());
                $prjOut = ms_newProjectionObj("proj=latlong");
                $llClick->project($prjIn, $prjOut);    ## <<----- Lat long of map click

                ## Determine parallels of new projection
                
                $ll_extent = $mapmgr->getLLExtent();
                $lat_1 = $ll_extent->miny + 5*($ll_extent->maxy - $ll_extent->miny)/6;
                $lat_2 = $ll_extent->miny + ($ll_extent->maxy - $ll_extent->miny)/6;
                $lat_0 = $lat_2 - ($lat_1 - $lat_2)/2;

                ## ----------------------------------------------------------------------
                $zoomval = $maction->getValue("zoom_fact");
                $rtn = $map->zoompoint($zoomval, $pixClick, $map->width, $map->height, $cur);
            }
            elseif ($maction->getValue("zoom_mode") == "rb_zou") {      ## Zoom out
                $zoomval = -1 * $maction->getValue("zoom_fact");
                $rtn = $map->zoompoint($zoomval, $pixClick, $map->width, $map->height, $cur);
            }
            elseif ($maction->getValue("zoom_mode") == "rb_pan") {      ## Pan
                $zoomval = 1;
                $rtn = $map->zoompoint($zoomval, $pixClick, $map->width, $map->height, $cur);
            }

            ##  ----- Query point on map  -----------------
            elseif ($maction->getValue("zoom_mode") == "rb_idn") {      ## Identify
                $wClick = ms_newPointObj();   // clicked point in world units
                $wClick->setXY($xwClick,$ywClick);

                $actLayer = @$map->getLayerByName($maction->getValue("active_layer"));
                if (@$actLayer->queryByPoint($wClick, MS_SINGLE, -1) == MS_SUCCESS) {
                    $querySet->setLayer ($actLayer);
                }
                $pixClick->setXY($map->width/2, $map->height/2);
                $zoomval = 1;
                $rtn = $map->zoompoint($zoomval, $pixClick, $map->width, $map->height, $cur);

                ## Capture query messages:
                if (! ($n=$querySet->getNumResults())) array_push($errorMsgs, "No selections returned");
                else array_push($errorMsgs, $n . " selections returned");
            }

            #$rtn = $map->zoompoint($zoomval, $pixClick, $map->width, $map->height, $cur);
        }

        ### Mouse click on reference map
        elseif ($maction->getAction() == "REFPOINT") {
            $tmppoint = $maction->getValue("refpoint");
            $refMap = $map->reference;

            // Map the reference map pixel coords to world coords
            $xwClick = $refMap->extent->minx
                + $tmppoint->x
                *($refMap->extent->maxx - $refMap->extent->minx)
                /$refMap->width;

            $ywClick = $refMap->extent->miny
                + ($refMap->height-$tmppoint->y)
                *($refMap->extent->maxy - $refMap->extent->miny)
                /$refMap->height;
                
            ## See if a reprojection is required
            $projIn = $mapmgr->getOrigProj();   ## get projection of the reference map
            $projOut= $map->getProjection();    ## get projection of the map
            if ($projIn != $projOut) {   ## If these projections are not the same, then reproject
                $wpt = ms_newPointObj($xwClick, $ywClick);
            }

            ## Compute the xy pixel locations in the main map that correspond to the xy pixels
            ## that were clicked in the reference map
            $xpixClick = ($xwClick - $cur->minx)
                        * $map->width/($cur->maxx - $cur->minx);
            $ypixClick = $map->height - ($ywClick - $cur->miny)
                        * $map->height/($cur->maxy - $cur->miny);

            #echo "<!-- world xy: " . $xwClick . "  " . $ywClick . "  pixel xy: " . $xpixClick . " " . $ypixClick . "  -->\n";
            #echo "<!-- clicked pixel: " . $tmppoint->x . " " . $tmppoint->y . "  -->\n";

            $pixClick = ms_newPointObj();
            $pixClick->setXY($xpixClick, $ypixClick);

            $zoomval = 1;

            $rtn = $map->zoompoint($zoomval, $pixClick, $map->width, $map->height, $cur);//, $ext);

            // Query the map object for the updated extents and save them
            ####$mapSetup->setExtents("nxt", $map->extent->minx, $map->extent->miny,
            ####            $map->extent->maxx, $map->extent->maxy);
            #$map->setProjection($mapSetup->getProjString('cur'));
        }

        ## Rubber band rectangle describing extents
        elseif ($maction->getAction() == "MAPRECT") {
            $tmpRect = $maction->getValue("maprect");

            ## 3D display using rectangle extents
            if ($maction->getValue('zoom_mode') == "3d") {
                // World coordinates of clicked point

                $wRect = ms_newRectObj();
                px2wRect($tmpRect, $wRect, $map, $cur);
                $queryResults->setQWindow($wRect);

                $imageName = "t3d_" . $basename . ".png";    // 3D image name
                $pageName  = "t3d_" . $basename . ".html";   // popup page name
                $urlDir    = $map->web->imageurl;
                $path      = $map->web->imagepath;          // Temporary file dir

                $outtmpPath = $map->web->imagepath . "tmp_" . $basename . ".png";
                $popupWin = new Popup3DWin();
                $popupWin->setURL($urlDir);
                $popupWin->setTempDir($path);
                $popupWin->setPage($pageName);
                $popupWin->setImage($imageName);
                $popupWin->setActive(true);

                ## Select an appropriate resolution so we don't have too
                ## many pixels to work with in the 3D display.
                $xwlng = $wRect->maxx - $wRect->minx;
                $ywlng = $wRect->maxy - $wRect->miny;

                ## Get ratio of requested length to total length
                $ratio = $xwlng < $ywlng
                        ? $ywlng/($orig->maxy - $orig->miny) : $xwlng/($orig->maxx - $orig->minx);

                ### Find the file with a resolution such that res*ratio < maxPx
                
                
                ##$demres = split('[\t\n \r]+', $map->getMetaData("DemResol"));
                $demres = split(',', $map->getMetaData("DemResol"));
                
                $maxPx=150;
                $i = 0;
                while ( $i < count($demres) && $demres[$i]*$ratio > $maxPx ) {
                    $i++;
                }
                if ($i == count($demres)) $i = count($demres)-1;
                $ext = pow(2, $i);
                #$demfile = $map->getMetaData("DemModel");
                ##echo "ext=".$ext;
                $demfile = ereg_replace(
                        '\.',
                        '_' . $ext . '\.',
                        $map->getMetaData("DemModel"));
                
                $vscale = .02/$ext;
                ##echo "ext=".$ext. "    ". $demfile;

                ### Project the selected map area to 3D by running demto3d
                ### Usage:  demto3d -win xUL yUL xLR yLR
                $cmd = "";
                $cmd .= 'PATH=${PATH}:/home/rob/work/vtkwork/usgsdem';
                $cmd .= ':/home/rob/local/bin;export PATH;';
                $cmd .= 'export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}';
                $cmd .= ':/home/rob/local/lib:/data1/vtk/VTK4.2/lib/vtk;';
                $cmd .= "demto3d -win " . $wRect->minx . " " . $wRect->maxy . " "
                    . $wRect->maxx . " " . $wRect->miny;
                ##$cmd .= " -input " . $map->getMetaData("DemModel");
                $cmd .= " -input " . $demfile;
                $cmd .= " -output " . $path . $imageName;
                $cmd .= " -z1 150 -z2 220 -vscale " . $vscale;
                $cmd .= " -tmp " . $outtmpPath;
                $cmd .= " -v 2>> " . $map->web->log;

                ##echo $cmd . "\n";
                ##echo "logfile: " . $map->web->log . "<br />\n";
            
                ### -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
                ### Debug prints to log file
                $outfile = fopen($map->web->log,  "a");
                $filewrite = fwrite($outfile, $cmd."\n");
                $filewrite = fwrite($outfile, "Rectangle  minx   miny   maxx   maxy\n");
                $filewrite = fwrite($outfile,
                    "  image    "
                    .$tmpRect->minx."    ".$tmpRect->miny."    "
                    .$tmpRect->maxx."    ".$tmpRect->maxy." \n");
                $filewrite = fwrite($outfile,
                    "  world    "
                    .$wRect->minx."    ".$wRect->miny."    "
                    .$wRect->maxx."    ".$wRect->maxy." \n");
                fclose($outfile);
                ### -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

                `$cmd`;

                ## Use same extents for map
                ####$mapSetup->setExtents("nxt", $cur->minx, $cur->miny, $cur->maxx, $cur->maxy);
            }  ### End of 3D display  ###
            
            ##  Query using rectangle extents  ###

            elseif ($maction->getValue('zoom_mode') == "rb_idn") {
                $wClick = ms_newPointObj();   // clicked point in world units
                ###$olayer = $map->getLayerByName($gbActiveLayer);
                ##echo $maction->getValue("active_layer");
                $actLayer = @$map->getLayerByName($maction->getValue("active_layer"));
                // World coordinates of rectangle
                $tmpRect = $maction->getValue("maprect");
                $wRect = ms_newRectObj();
                px2wRect($tmpRect, $wRect, $map, $cur);
                $querySet->setExtent($wRect);


                if (@$actLayer->queryByRect($wRect) == MS_SUCCESS) {   // Note: @ suppresses err msg
                    $querySet->setLayer ($actLayer);
                }
                ## Capture query messages:
                if (! ($n=$querySet->getNumResults())) array_push($errorMsgs, "No selections returned");
                else array_push($errorMsgs, $n . " selections returned");

                $zoomval = 1;
            } ### End of Query by rectangle ###

        ## Default behavior of rubber band box is to zoom in
            else {
                $zoomRect = ms_newRectObj();
                ## Adjust x or y scale to fit into output image width and height
                ## Take the smaller of xscale or yscale
                $zoomRect->setExtent(
                        $tmpRect->minx, $tmpRect->miny, $tmpRect->maxx, $tmpRect->maxy);
                $zoomRect->fit($map->width, $map->height);

                $map->zoomrectangle ($zoomRect, $map->width, $map->height, $cur);
            }
        }


        #### Pan or recenter map                      ##########

        elseif ($maction->getAction() == "PAN") {

            $new = ms_newRectObj();
            $cur = $map->extent;

            ##echo "mapaction=" . $maction->getAction() . "  extent: " . $cur->minx . " " . $cur->miny
            ##     . " " . $cur->maxx . " " . $cur->maxy . "<br />\n";
            if ($maction->getValue("pan") == "NW") {
                $new->minx = $cur->minx - ($cur->maxx - $cur->minx);
                $new->maxx = $cur->minx;
                $new->miny = $cur->maxy;
                $new->maxy = $cur->maxy + ($cur->maxy - $cur->miny);
            }
            elseif ($maction->getValue("pan") == "N") {
                $new->minx = $cur->minx;
                $new->maxx = $cur->maxx;
                $new->miny = $cur->maxy;
                $new->maxy = $cur->maxy + ($cur->maxy - $cur->miny);
            }
            elseif ($maction->getValue("pan") == "NE") {
                $new->minx = $cur->maxx;
                $new->maxx = $new->minx + ($cur->maxx - $cur->minx);
                $new->miny = $cur->maxy;
                $new->maxy = $cur->maxy + ($cur->maxy - $cur->miny);
            }
            elseif ($maction->getValue("pan") == "W") {
                $new->minx = $cur->minx - ($cur->maxx - $cur->minx);
                $new->maxx = $cur->minx;
                $new->miny = $cur->miny;
                $new->maxy = $cur->maxy;
            }
            elseif ($maction->getValue("pan") == "E") {
                $new->minx = $cur->maxx;
                $new->maxx = $new->minx + ($cur->maxx - $cur->minx);
                $new->miny = $cur->miny;
                $new->maxy = $cur->maxy;
            }
            elseif ($maction->getValue("pan") == "SW") {
                $new->minx = $cur->minx - ($cur->maxx - $cur->minx);
                $new->maxx = $cur->minx;
                $new->miny = $cur->miny - ($cur->maxy - $cur->miny);
                $new->maxy = $cur->miny;
            }
            elseif ($maction->getValue("pan") == "S") {
                $new->minx = $cur->minx;
                $new->maxx = $cur->maxx;
                $new->miny = $cur->miny - ($cur->maxy - $cur->miny);
                $new->maxy = $cur->miny;
            }
            elseif ($maction->getValue("pan") == "SE") {
                $new->minx = $cur->maxx;
                $new->maxx = $new->minx + ($cur->maxx - $cur->minx);
                $new->miny = $cur->miny - ($cur->maxy - $cur->miny);
                $new->maxy = $cur->miny;
            }

            $map->setExtent($new->minx, $new->miny, $new->maxx, $new->maxy);
        }
        elseif ($maction->getAction() == "LLZOOM") {
            // Zoom to the requested latlong point and center map around it
            $lon = $maction->getValue("lon");
            $lat = $maction->getValue("lat");

            ## Define a new projection
            $zone = floor((180+$lon)/6) + 1;
            $projstr = "proj=utm,ellps=GRS80,zone=" . $zone;
            $projIn = ms_newProjectionObj("proj=latlong");
            $projOut = ms_newProjectionObj($projstr);

            ## Project the requested point and define new extents around it
            $pt = ms_newPointObj();
            $pt->setXY($lon, $lat);
            $pt->project($projIn, $projOut);
            $new = ms_newRectObj();

            $new->minx = $pt->x - 200000;
            $new->maxx = $pt->x + 200000;
            $new->miny = $pt->y - 200000*$map->height/$map->width;   ## consider aspect ratio
            $new->maxy = $pt->y + 200000*$map->height/$map->width;
            $map->setProjection($projstr, MS_FALSE);

            $map->setExtent($new->minx, $new->miny, $new->maxx, $new->maxy);
        }
        elseif ($maction->getAction() == "STATEZOOM") {
            if ($maction->getValue('county') == 'all')
                $extNew = getStateExtent($db, $maction->getValue('state'));
            else
                $extNew = getCountyExtent($db, $maction->getValue('county'));
            zoomToExtent($map, $extNew);
        }

        elseif ($maction->getAction() == "REGZOOM") {
            $extNew = getRegionExtent($db, $maction->getValue('region'));
            zoomToExtent($map, $extNew);
        }

        elseif ($maction->getAction() == "TRZOOM") {
            $extNew = getTwnrngExtent($db, $maction->getValue('meridian'),
                      $maction->getValue('twnnum'), $maction->getValue('twndir'),
                      $maction->getValue('rngnum'), $maction->getValue('rngdir'));
            if ($extNew)
                zoomToExtent($map, $extNew);
            else  {
                $maction->setAction('ERROR');   ## This probably isn't necessary
                array_push($errorMsgs, "Meridian, township, range specified was not found");
            }
        }

        elseif ($maction->getAction() == "QUERYATTR") {
            $actLayer = @$map->getLayerByName($maction->getValue("active_layer"));
            if (!$actLayer) return;
         ## if ( ($rtn=@$actLayer->queryByAttributes(
         ##     $maction->getValue("dummy"), $maction->getValue("sql"), MS_MULTIPLE)) == MS_SUCCESS) {
##### Note hardcoded gid column. Need to get the field name from metadata
            if ( ($rtn=@$actLayer->queryByAttributes( "gid",$maction->getValue("sql"), MS_MULTIPLE)) == MS_SUCCESS) {
                    $querySet->setLayer ($actLayer);
            }
                ## Capture query messages:
                $n = $querySet->getNumResults();
                if (!isset($n)) $n = 0;
                if ($n == 0) {
                    array_push($errorMsgs, "No selections returned");
                    $n = 0;
                }
                else array_push($errorMsgs, $n . " selections returned");


                #if (! ($n=$querySet->getNumResults())) array_push($errorMsgs, "No selections returned");
                #else array_push($errorMsgs, $n . " selections returned");
                            #######echo "rtn=".$rtn.", n=".$n."<br />\n";
        }

        if ($maction->getAction() == "ERROR") {
            #$mapSetup->setExtents("nxt", $cur->minx, $cur->miny, $cur->maxx, $cur->maxy);
            #$map->setExtent($cur->minx, $cur->miny, $cur->maxx, $cur->maxy);
            #$map->setProjection($mapSetup->getProjString('cur'));
        }
    }

?>
