<?php
### ==========================================================================
###   $Id: giserv.php,v 1.1.1.1 2007/04/24 02:04:23 rob Exp $
###   $Name:  $
### ==========================================================================

    dl('php_mapscript.so');
    require_once 'widget.inc';
    require_once 'tree.inc';
    require_once 'defs.inc';
    require_once 'userlogin.inc';
    require_once 'mapmgr.inc';
    require_once 'mapcmds.inc';
    require_once 'query.inc';
    require_once 'DB.php';

    #####################################################################################################
    $gbErrorMsgs = array();   ## Initialize array for global error messages
    ## Establish database connection
    $gbUConn = new UserLogin();
    if ( !($gbDB = $gbUConn->checkConnection()) ) {
        ## Get the correct extension. We've been using both phtml and php
        header( 'Location: giserv_login.'
            . preg_replace('/^.*\.(\w+)$/', '\1', $_SERVER['PHP_SELF'])
            . '?redir_error=Connection error, please try again' );
    }
  
    $gbMapmgr = new MapManager($gbDB, 'tulsa.map', $gbUConn);
    if (count($_POST) > 0) $gbMapmgr->setFromRequest($_POST);

    $gbMap = &$gbMapmgr->getMapObject();
    #####################################################################################################

    $gbKWPairs = array();

    $qyPanel = crQueryPanel($_POST, $gbKWPairs);
    $qzPanel = crQZoomPanel($_POST, $gbKWPairs);
    $pzPanel = crPZoomPanel($_POST, $gbKWPairs);
    $vwPanel = crViewPanel($_POST, $gbKWPairs, $gbMapmgr->getViewNameList());
    ##  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    ## Set up layer panel. This is a collapsing tree widget
    $tree = new Tree("rootNode", "root", "root");
    $tree->htmlName = "rootNode";
    $lyPanel = crLayerPanel($_POST, $gbKWPairs, $tree, $gbMapmgr->getMapObject());
    ##  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    ## define a zero length array for these empty panels
    $emptyArray = array();

    foreach ($gbKWPairs as $i => $u) {
        ##echo "<!--  gbKWPairs: " . $gbKWPairs[$i]->getName() . "=" . $gbKWPairs[$i]->getValue() . "  -->\n";
        $gbKWPairs[$i]->validate();
    }

    ## Initialize Some arrays, option lists that require database connection
    initOptions($gbDB, $gbKWPairs);

    if (count($_POST) == 0)   ## For initial page, copy map layer settings to widgets
        mapToLayerPanel($gbMap, $gbKWPairs);

    ###dbgPrintKW($gbKWPairs);   ## <<<<<<   Debug print

    $mouseAction = new MouseAction();
    getMouseAction($_POST, $gbKWPairs, $mouseAction);

    $gbQuerySet = new QuerySet();
    if (!$mouseAction->getError())
        execCommand($gbUConn, $mouseAction, $gbKWPairs, $gbMapmgr, $gbQuerySet, $gbErrorMsgs);
    else
        echo "Error from mouseAction<br />\n";

    $gbQuerySet->crOverlays($gbMap, $gbErrorMsgs);
 
### ==========================================================================

    function dbgPrintKW(&$kw) {
        foreach ($kw as $i=>$u)
            print "kw pair: " . $i . " : "
                . $kw[$i]->name . " = "
                . $kw[$i]->getValue() . "<br />\n";
    }
### ==========================================================================

    function initOptions(&$db, &$kw) {


        ## Get code of state name that will display in option list
        if ( isset($kw['sel_qzstate']) ) {
            $stcode =  $kw['sel_qzstate']->getValue();
            $sql = "select c.state_n_code || c.cnty_n_code as fips_code, c.cnty_name as cnty_name"
                 . " from fips_counties c, fips_states s"
                 . " where c.state_n_code = s.state_n_code"
                 . " and s.state_a_code = '" . $kw['sel_qzstate']->getValue() . "'"
                 . " order by c.cnty_name";

            
            $result = $db->query($sql);
            if (DB::iserror($result))   die ($result->getMessage());

            $cntyArray = array('all' => '- Whole State -');
            while ($row = $result->fetchRow()) {
                if (DB::isError($row))  die ($row->getMessage());
                $cntyArray[$row['fips_code']] = $row['cnty_name'];
            }
            $result->free();

            ## Pass to the option list
            $kw['sel_qzcnty']->setValidList($cntyArray);
        }


        ##
    }

    #####################################################################################################
### ==========================================================================

    function & crQueryPanel(&$rq_array, &$kw) {

        ## - - -  ## Layer select panel

        $sel = new TextListBox("sel_qy", "Select Layer For Query",
                    "sel_list", "Select layer to be queried or identified",
                    DF_LABEL_LEFT, DF_BLOCK_P, DF_CHR, 1, null);
            $sel->setBlockClass("rb_pnl_sel");
            $sel->setJSEvent("onchange");
            $sel->setJSMethod("selSubPanel('pnl_qy','sel_qy');");
            $sel->add("pnl_qy_seis", "2D Seismic");
            $sel->add("pnl_qy_soil", "Soil Samples");
            ### disable:  $sel->add("pnl_qy_river", "Rivers");

        
        $pnl_qsel = new Panel("pnl_qy_rb", null, null);  ## Create the panel
        $pnl_qsel->add($sel);       ## Add selection box to panel

        ## Create subpanels for each layer that is queryable
        $pnl_qseis = crQueryPanel_cseis($kw);
        $pnl_qsoil  = crQueryPanel_soil($kw);
        ### disable:  $pnl_qriver = crQueryPanel_river($kw);


        ## ==== ## main panel
        $panel = new Panel( "pnl_qy", "Query", "title_pnl");
                   ########  submit button:
                   #### <p id="pnl_qy_sub"><input type="submit" class="sub_btn" id="sub_qy"

        $panel->add($pnl_qsel);
        $panel->add($pnl_qseis);
        $panel->add($pnl_qsoil);
        ### disable:  $panel->add($pnl_qriver);

        $btn = new Button("sub_qy", "Run Query", "sub_btn", "Click to submit query", 
                DF_LABEL_OVER, DF_BLOCK_NONE);
        $pnl_sub = new Panel("pnl_qy_sub", null, null);
        $pnl_sub->add($btn);
        $panel->add($pnl_sub);

        $panel->setValuesFromRequest($rq_array);
        $panel->setKeyWidgetPairs($kw);
        return $panel;
    }

    function &crQueryPanel_cseis(&$kw) {

        ## - - - ## Seismic layer query panel

        $txt_lineid = new TextField("q_lineid", "Line ID", null,
                "Seismic line ID", DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 20, null);
        $txt_lineid->setBlockClass("input1");
        $txt_yearsh = new TextField("q_yearshot", "Year Shot", null,
                "Year seismic line was shot", DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 20, null);
        $txt_yearsh->setBlockClass("input1");
        $txt_compan = new TextField("q_comp", "Company", null,
                "Company that owns data", DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 20, null);
        $txt_compan->setBlockClass("input1");
        
        $rb_segy = new RadioButtonGroup("q_segy", "Segy file available?", "input1",
                "Is a SEGY file is available for this seismic line?",
                 DF_LABEL_TOP, DF_BLOCK_P, DF_LAYOUT_HORIZ);
            $rb_segy->add("yesorno", "Don&#039;t care",
                          "Include seismic lines regardless if there are SEGY files available");
            $rb_segy->add("yes", "Yes",
                          "Include seismic lines only if there is are SEGY files available");
            $rb_segy->add("no", "No",
                          "Include seismic only if there are no SEGY files available");

        $rb_img  = new RadioButtonGroup("q_pimage", "Image avalailable?", "input1",
                "Is a processed image available for this seismic line?",
                 DF_LABEL_TOP, DF_BLOCK_P, DF_LAYOUT_HORIZ);
            $rb_img->add("yesorno", "Don&#039;t care",
                          "Include seismic lines regardless if there are processed images available");
            $rb_img->add("yes", "Yes",
                          "Include seismic lines only if there are processed images available");
            $rb_img->add("no", "No",
                          "Include seismic lines only if there is no processed images available");

        $pnl_qseis = new Panel("pnl_qy_seis", null, null);
        $pnl_qseis->add($txt_lineid);
        $pnl_qseis->add($txt_yearsh);
        $pnl_qseis->add($txt_compan);
        $pnl_qseis->add($rb_segy);
        $pnl_qseis->add($rb_img);
        return $pnl_qseis;
    }

    function &crQueryPanel_soil(&$kw) {

        ## - - - ## Soil sample layer query panel
        # $name, $label, $cssClass, $title, $labelPos, $blockType, $layout, $nrows
        $grd_soil = new InputGrid("q_soil_grd", null, "tb_inp", null,
                DF_LABEL_NONE, DF_BLOCK_NONE, DF_LAYOUT_ARRAY, 4);

        $sel_elem = new TextListBox("q_sel_elem", "Element", "sel_list", null,
                        DF_LABEL_NONE, DF_BLOCK_TD, DF_CHR, 1, null);
            $sel_elem->setBlockClass("td_inp");
            $sel_elem->add("al_pct","Aluminum (pcent)");
            $sel_elem->add("as_ppm","Arsenic (ppm)");
            $sel_elem->add("fe_pct","Iron (pcent)");
            $sel_elem->add("k_pct","Potassium (pcent)");

        $txt_min = new TextField("q_txt_minelem", "Minimum",null, null,
                DF_LABEL_NONE, DF_BLOCK_TD, DF_DBL, 5, null);
            $txt_min->setBlockClass("td_inp");
        $txt_max = new TextField("q_txt_maxelem", "Maximum",null, null,
                DF_LABEL_NONE, DF_BLOCK_TD, DF_DBL, 5, null);
            $txt_max->setBlockClass("td_inp");
        $grd_soil->add($sel_elem);
        $grd_soil->add($txt_min);
        $grd_soil->add($txt_max);

        $pnl_qsoil = new Panel("pnl_qy_soil", null, null);
            $pnl_qsoil->add($grd_soil);

        return $pnl_qsoil;
    }


    function &crQueryPanel_river(&$kw) {
        ## - - - ## River layer query panel

        $txt_rvname = new TextField("q_txt_nameriv", "Name", null,
                "Name of river to select", DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 20, null);
        $txt_rvname->setBlockClass("input1");
        $pnl_qriver = new Panel("pnl_qy_river", null, null);
        $pnl_qriver->add($txt_rvname);
        return $pnl_qriver;
    }

    ## ======================================================================================================

    function lyRowSetup(&$tree, &$map) {

        ##$tree = new Tree("rootNode","root","root");
        $lyrGroups = null;

        ## Capture predefined layer groups
        if ($map->getMetaData("layergroups") && $map->getMetaData("layergroups") != "") {
            $layergroups = split('[ ]*,[ ]*', $map->getMetaData("layergroups"));
            $lyrGroups = array();  ## mapping of layer name to layer (title or label)
            foreach ($layergroups as $g) {
                list($key,$label) = split('[ ]*=[ ]*', $g);
                $lyrGroups[$key] = $label;
            }
        }

        for ($i=0; $i<$map->numlayers; $i++) {
            $layerObj = $map->getLayer($i);
            if ($layerObj->getMetaData("no_icon") == "Y") continue;    ### Don't make icon for this layer

            if ( ($group = $layerObj->getMetaData("layergroup")) != "" ) {
                ## This layer belongs to a group

                ## If the group doesn't exist, create it. Then add the group to the layer
                unset($grpNode);
                $grpNode =  &$tree->find($group);
                if (!$grpNode) {
                    $grpNode = new Tree( $group, $lyrGroups[$group], "group" );
                    $tree->add( $grpNode );
                }
                unset($lyrNode);
                $lyrNode = new Tree($layerObj->name, $layerObj->getMetaData("layerlabel"), "layer", $i);
                $grpNode->add( $lyrNode );
            }
            else {   ## Layer does not belong to a group
                     ## Add it to the root node
                unset($lyrNode);
                #####$tree->add( new Tree ($layerObj->name, $layerObj->getMetaData("layerlabel"), "layer", $i) );
                $lyrNode = new Tree ($layerObj->name, $layerObj->getMetaData("layerlabel"), "layer", $i);
                $tree->add( $lyrNode );
            }

            ## Create nodes for each class under the layer node
            unset($node);
            $node = &$tree->find($layerObj->name);

            if ($layerObj->numclasses == 1) {
                #$classObj = $layerObj->getClass(0);
                #$node->name = $classObj->name;
                $node->clsIdx = 0;
            }
            else {

                for ($j=0; $j<$layerObj->numclasses; $j++) {
                    $classObj = $layerObj->getClass($j);
                    unset($newTree);
                    $newTree = new Tree("ly_".$i."_".$j, $classObj->name, "class", $i, $j);
                    $node->add(  $newTree  );
                }
            }
        }
    }
    ## =================================================================================

    function & crLayerPanel( &$rq_array, &$kw, &$tree, &$map) {

        ## Get layers in tree structure
        lyRowSetup($tree, $map);

        ## ---- Main panel ----
        $panel = new Panel("pnl_ly", "Layer Control and Legend", "title_pnl");


        ## ---- Data panel ----
        $pnl_ly_data = new Panel("pnl_ly_data",null,null);

        $tbl_ly = new TableWidget("tbl_ly", null, "tbl_ly", null,DF_LABEL_NONE, DF_BLOCK_NONE);
        $tbl_ly->setCellspacing(0);

        ## ---- Row for column headers
        $row = new RowWidget("r_ly", null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE);

        $totDepth = $tree->getDepth();
        for ($i=0; $i<$totDepth-1; $i++)
            $row->add (new TextWidget("hdr".$i."_ly",null,null, DF_BLOCK_NONE));
 
        #$row->add (new TextWidget("hdr1_ly",null,null, DF_BLOCK_NONE));
        #$row->add (new TextWidget("hdr2_ly",null,null, DF_BLOCK_NONE));

        $row->add (new TextWidget("hdr".$i++."_ly","Visible", null, DF_BLOCK_NONE), "hdr1");
        $row->add (new TextWidget("hdr".$i++."_ly","Label", null, DF_BLOCK_NONE), "hdr1");
        $row->add (new TextWidget("hdr".$i++."_ly","Key", null, DF_BLOCK_NONE), "hdr3");
        $row->add (new TextWidget("hdr".$i++."_ly","Layer", null, DF_BLOCK_NONE), "hdr4");
        $tbl_ly->add($row);


        $tree->buildWidget($tbl_ly, 0, $totDepth, null);

        $pnl_ly_data->add($tbl_ly);

        ## ---- Add sub panels to the main panel
        $panel->add($pnl_ly_data);

        
        ### ---   Button Panel --- ###

        $btn = new Button("sub_ly", "Redraw Map", "sub_btn", "Click to redraw map with layer changes", 
                DF_LABEL_OVER, DF_BLOCK_NONE);
        $pnl_ly_sub = new Panel("pnl_ly_sub", null, null);
        $pnl_ly_sub->add($btn);

        $panel->add($pnl_ly_sub);

        $panel->setValuesFromRequest($rq_array);
        $panel->setKeyWidgetPairs($kw);
        return $panel;
    }
    ## =================================================================================



    function & crQZoomPanel( &$rq_array, &$kw) {

        $stateList = array(
            "AL" => "Alabama", "AK" => "Alaska", "AZ" => "Arizona", "AR" => "Arkansas", "CA" => "California", "CO" => "Colorado",
            "CT" => "Connecticut", "DE" => "Delaware", "DC" => "District of Columbia", "FL" => "Florida", "GA" => "Georgia",
            "HI" => "Hawaii", "ID" => "Idaho", "IL" => "Illinois", "IN" => "Indiana", "IO" => "Iowa",
            "KS" => "Kansas", "KY" => "Kentucky", "LA" => "Louisiana", "ME" => "Maine", "MD" => "Maryland",
            "MA" => "Massachusetts", "MI" => "Michigan", "MN" => "Minnesota", "MS" => "Mississippi", "MO" => "Missouri",
            "MT" => "Montana", "NE" => "Nebraska", "NV" => "Nevada", "NH" => "New Hampshire", "NJ" => "New Jersey",
            "NM" => "New Mexico", "NY" => "New York", "NC" => "North Carolina", "ND" => "North Dakota", "OH" => "Ohio",
            "OK" => "Oklahoma", "OR" => "Oregon", "PA" => "Pennsylvania", "RI" => "Rhode Island", "SC" => "South Carolina",
            "SD" => "South Dakota", "TN" => "Tennessee", "TX" => "Texas", "UT" => "Utah", "VT" => "Vermont",
            "VA" => "Virginia", "WA" => "Washington", "WV" => "West Virginia", "WI" => "Wisconsin", "WY" => "Wyoming",
            );
            asort($stateList);


        $regionList = array(
            "3" => "Colorado Plateau and Basin and Range",
            "8" => "Eastern",
            "6" => "Gulf Coast",
            "0" => "MMS Lease Block Area",
            "7" => "Midcontinent",
            "2" => "Pacific Coast",
            "4" => "Rocky Mountains and Northern Great Plains",
            "5" => "West Texas and Eastern New Mexico"
            );
            asort($regionList);

        $meridianList = array(
            array("coords"=>"24,48,33,24,34,8,42,9,67,12,62,34,60,41,55,58", "code"=>"33", "text"=>"Willamette"),
            array("coords"=>"24,48,71,63,64,98,59,106,48,92,46,102,27,98,22,72", "code"=>"21", "text"=>"Mount Diablo"),
            array("coords"=>"23,48,28,50,24,65,19,61,22,49", "code"=>"15", "text"=>"Humboldt"),
            array("coords"=>"28,98,46,101,48,94,60,105,55,120,46,118,38,110,29,99", "code"=>"27", "text"=>"San Bernardino"),
            array("coords"=>"67,13,64,33,60,42,57,56,86,63,90,47,83,50,78,43,74,29,70,15", "code"=>"08", "text"=>"Boise"),
            array("coords"=>"81,47,76,33,70,15,99,19,122,23,121,48,89,44", "code"=>"20", "text"=>"Principal"),
            array("coords"=>"90,72,82,71,82,77,91,78", "code"=>"30", "text"=>"Uintah"),
            array("coords"=>"63,93,71,61,85,62,83,69,94,71,91,97", "code"=>"26", "text"=>"Salt Lake"),
            array("coords"=>"85,101,89,101,89,106,84,105", "code"=>"22", "text"=>"Navajo"),
            array("coords"=>"55,120,65,93,91,98,87,134,76,133", "code"=>"14", "text"=>"Gila and Salt River"),
            array("coords"=>"94,59,96,54,100,55,100,59", "code"=>"34", "text"=>"Wind River"),
            array("coords"=>"89,45,86,71,95,71,93,86,110,88,113,100,166,103,164,86,158,73,151,65,121,63,120,48", "code"=>"06", "text"=>"Sixth Principal"),
            array("coords"=>"93,80,96,81,97,85,93,85,96,81", "code"=>"31", "text"=>"Ute"),
            array("coords"=>"93,87,87,134,105,131,119,132,120,101,111,96,111,88", "code"=>"23", "text"=>"New Mexico Principal"),
            array("coords"=>"122,21,168,25,165,41,180,58,186,67,182,73,180,80,191,100,183,122,182,127,171,127,166,106,164,86,157,71,152,63,138,62,137,42,120,42", "code"=>"05", "text"=>"Fifth Principal"),
            array("coords"=>"121,41,138,42,138,58,124,58,123,60,121,60", "code"=>"07", "text"=>"Black Hills"),
            array("coords"=>"122,104,122,100,137,101,136,104", "code"=>"11", "text"=>"Cimarron"),
            array("coords"=>"135,116,139,101,167,103,167,124,148,124", "code"=>"17", "text"=>"Indian"),
            array("coords"=>"168,127,183,126,183,135,181,146,187,150,179,150,170,148", "code"=>"18", "text"=>"Louisiana"),
            array("coords"=>"168,27,186,29,177,35,199,45,195,63,185,65,174,54,166,45", "code"=>"46", "text"=>"Fourth Principal Extended"),
            array("coords"=>"183,84,179,76,187,66,190,71,183,84", "code"=>"04", "text"=>"Fourth Principal"),
            array("coords"=>"192,98,185,84,192,71,195,65,198,69,199,90,192,98", "code"=>"03", "text"=>"Third Principal"),
            array("coords"=>"197,123,186,118,188,113,199,114,197,123", "code"=>"09", "text"=>"Chickasaw"),
            array("coords"=>"186,133,185,120,198,122,199,133,186,133", "code"=>"10", "text"=>"Choctaw"),
            array("coords"=>"192,142,185,143,185,134,192,142", "code"=>"32", "text"=>"Washington"),
            array("coords"=>"186,145,197,148,191,151,186,145", "code"=>"24", "text"=>"St. Helena"),
            array("coords"=>"198,125,200,112,212,110,216,123,198,125", "code"=>"16", "text"=>"Huntsville"),
            array("coords"=>"193,145,189,135,201,132,200,125,217,125,216,138,204,139,203,143,193,145", "code"=>"25", "text"=>"St. Stephens"),
            array("coords"=>"205,144,207,139,226,138,238,136,249,157,253,173,246,172,235,161,232,146,224,144,220,146,216,143,205,144", "code"=>"29", "text"=>"Tallahassee"),
            array("coords"=>"202,68,202,54,205,46,207,41,199,45,184,38,193,33,202,35,215,35,220,49,223,59,221,66,202,68", "code"=>"19", "text"=>"Michigan"),
            array("coords"=>"197,94,199,70,207,68,213,82,210,91,209,92,197,94", "code"=>"02", "text"=>"Second Principal"),
            array("coords"=>"220,73,215,77,213,69,220,67,220,73", "code"=>"01", "text"=>"First Principal"),
            array("coords"=>"222,73,228,72,228,77,223,78,222,73", "code"=>"35", "text"=>"U.S. Military"),
            array("coords"=>"70,143,56,146,34,144,48,132,70,143", "code"=>"45", "text"=>"Umiat"),
            array("coords"=>"55,166,34,166,32,154,34,145,56,148,55,166", "code"=>"44", "text"=>"Kateel River"),
            array("coords"=>"75,158,57,166,58,149,72,143,75,158", "code"=>"13", "text"=>"Fairbanks"),
            array("coords"=>"68,176,57,193,22,207,42,183,29,178,32,167,67,166,68,176", "code"=>"28", "text"=>"Seward"),
            array("coords"=>"111,184,103,190,88,177,71,175,68,164,76,160,81,170,94,172,111,184", "code"=>"12", "text"=>"Copper River"),
       );

        ## Create Zoom Type selection panel            ###############

        $sel = new TextListBox("sel_qz", "Select Zoom Type",
                    "sel_list", "Select the type of pre-defined zoom to use",
                    DF_LABEL_LEFT, DF_BLOCK_P, DF_CHR, 1, null);
            $sel->setBlockClass("rb_pnl_sel");
            $sel->setJSEvent("onchange");
            $sel->setJSMethod("selSubPanel('pnl_qz','sel_qz');");
                    
            $sel->add("pnl_qz_ll", "Latitude/Longitude");
            $sel->add("pnl_qz_st", "State/County");
            $sel->add("pnl_qz_rg", "Geologic Region");
            $sel->add("pnl_qz_tr", "Township/Range");

        $pnl_zsel = new Panel("pnl_qz_rb", null, null);  ## Create the panel
        $pnl_zsel->add($sel);       ## Add selection box to panel


        ## Create Lat Long zoom definition panel            ###############

        $txt_lat = new TextField("txt_qzlat", "Latitude", null,
                "Enter the latitude of the location for zooming in",
                DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 15, null);
            $txt_lat->setBlockClass("input1");
        $txt_lon = new TextField("txt_qzlon", "Longitude", null,
                "Enter the longitude of the location for zooming in",
                DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 15, null);
            $txt_lon->setBlockClass("input1");
        $pnl_zll = new Panel("pnl_qz_ll", null, null);
        $pnl_zll->add($txt_lat);
        $pnl_zll->add($txt_lon);


        ## Create State County defintion panel            ###############

        $sel_st = new TextListBox("sel_qzstate", "State",
                    "sel_list", "Select the state for zooming",
                    DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 1, null);
        $sel_st->setBlockClass("input1");
        $sel_st->setJSEvent("onchange");
        $sel_st->setJSMethod("loadCounty(this.form.sel_qzcnty, this.form.sel_qzstate);");

        foreach ($stateList as $i=>$u)   ## Put state code/names into select list
            $sel_st->add($i, $u);
                    

        $sel_ct = new TextListBox("sel_qzcnty", "County",
                    "sel_list", "Select the county for zooming",
                    DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 1, null);
        $sel_ct->setBlockClass("input1");
        $sel_ct->add("01001","Autauga");  ## Just add a bogus entry (Javascript will populate this select list)

        ## Disable validation since it will always fail since complete list is only in javascript
        $sel_ct->setNoValidate();

        $pnl_stc = new Panel("pnl_qz_st", null, null);
        $pnl_stc->add($sel_st);
        $pnl_stc->add($sel_ct);

        ## Create Geological region sub panel            ###############


        $sel_rg= new TextListBox("sel_qzregion", "Region",
                    "sel_list", "Select region to zoom in to",
                    DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 1, null);
        $sel_rg->setBlockClass("input1");

        foreach ($regionList as $i=>$u)
            $sel_rg->add($i, $u);

        $pnl_zrg = new Panel("pnl_qz_rg", null, null);
        $pnl_zrg->add($sel_rg);


        ## Create Township/Range sub panel                 ###############

        #            ### Meridian list
        $pw1 = new PWidget("pw1", null, "input1");
        $sel_qzmer = new TextListBox("sel_qzmer", "Select meridian from list or by clicking area on map", "sel_list", "Select Meridian",
                         DF_LABEL_TOP, DF_BLOCK_NONE, DF_CHR, 1, null);
        $sel_qzmer->setJSEvent("onchange");
        $sel_qzmer->setJSMethod("updTRInfo();");

        uasort($meridianList, "sortMeridByText");
        foreach ($meridianList as $i=>$u)
            $sel_qzmer->add($meridianList[$i]["code"], $meridianList[$i]["text"]);

        $pw1->add($sel_qzmer);      ###------------


        #            ###   <p>   Image map for selecting meridian.  Use ImageWidget

        $iw = new ImageWidget("meridian_map", null, null, DF_BLOCK_P, "imagedir/meridians_imap_idx.png", 300,215, "Meridian zones");
        $iw->setBlockClass("imagemaps");
        $imap = new ImageMap("merididx", "imapTitle", null, "imapSelect", $meridianList);
        for ($i=0; $i<count($meridianList); $i++)
            $imap->addArea($meridianList[$i]["code"], $meridianList[$i]["text"], $meridianList[$i]["coords"]);

        $iw->setImageMap($imap);

        $pw1->add($iw);


        #            ###   <p>   Township group
        $pw2 = new PWidget("pw2", null, "input1");
        $tw_qztwn = new TextWidget("tw_qztwn", "Township", null, DF_BLOCK_NONE);
        $txt_qztwn_out = new TextField("txt_qztwn_out", null, "txt_ro", null, DF_LABEL_NONE,
                        DF_BLOCK_NONE, DF_CHR, 25, "(1-20)");
        $txt_qztwn_out->setReadonly(true);
        $txt_qztwn_out->setNBr(0,1);   ## add <br> at end
 


        $txt_qztwn     = new TextField("txt_qztwn", "null", null, "Enter township number", DF_LABEL_NONE,
                        DF_BLOCK_NONE, DF_CHR, 4, null);
        $txt_qztwn->setJSEvent("onchange");
        $txt_qztwn->setJSMethod("twnGrp.validate();");

        $rbg1 = new RadioButtonGroup("ch_qztwn", null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE, DF_LAYOUT_HORIZ);
        $rbg1->add("N", "N", null);
        $rbg1->add("S", "S", null);
        $rbg1->setJSEvent("onclick");
        $rbg1->setJSMethod("twnGrp.update();");

        $txt_qztwn_msg = new TextField("txt_qztwn_msg", null, "txt_err", null, DF_LABEL_NONE,
                        DF_BLOCK_NONE, DF_CHR, 25, null);
        $txt_qztwn_msg->setReadonly(true);
        $ch_qztwn_h = new TextField("ch_qztwn_h", null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE, DF_CHR, 0, null);
        $ch_qztwn_h->setHidden(true);

        $pw2->add($tw_qztwn);
        $pw2->add($txt_qztwn_out);
        $pw2->add($txt_qztwn);
        $pw2->add($rbg1);
        $pw2->add($txt_qztwn_msg);
        $pw2->add($ch_qztwn_h);

        ##   </p>    ###


        #            ###   <p>   Range group
        $pw3 = new PWidget("pw3", null, "input1");
        $tw_qzrng = new TextWidget("tw_qzrng", "Range", null, DF_BLOCK_NONE);
        $txt_qzrng_out = new TextField("txt_qzrng_out", null, "txt_ro", null, DF_LABEL_NONE,
                        DF_BLOCK_NONE, DF_CHR, 25, "(1-20)");
        $txt_qzrng_out->setReadonly(true);
        $txt_qzrng_out->setNBr(0,1);   ## add <br> at end
        $txt_qzrng     = new TextField("txt_qzrng", "null", null, "Enter range number", DF_LABEL_NONE,
                        DF_BLOCK_NONE, DF_CHR, 4, null);
        $txt_qzrng->setJSEvent("onchange");
        $txt_qzrng->setJSMethod("rngGrp.validate()");

        $rbg1 = new RadioButtonGroup("ch_qzrng", null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE, DF_LAYOUT_HORIZ);
        $rbg1->add("E", "E", null);
        $rbg1->add("W", "W", null);
        $rbg1->setJSEvent("onclick");
        $rbg1->setJSMethod("rngGrp.update();");

        $txt_qzrng_msg = new TextField("txt_qzrng_msg", null, "txt_err", null, DF_LABEL_NONE,
                        DF_BLOCK_NONE, DF_CHR, 25, "Invalid value");
        $txt_qzrng_msg->setReadonly(true);
        $ch_qzrng_h = new TextField("ch_qzrng_h", null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE, DF_CHR, 0, null);
        $ch_qzrng_h->setHidden(true);

        $pw3->add($tw_qzrng);
        $pw3->add($txt_qzrng_out);
        $pw3->add($txt_qzrng);
        $pw3->add($rbg1);
        $pw3->add($txt_qzrng_msg);
        $pw3->add($ch_qzrng_h);

        ##   </p>    ###



        $pnl_ztr = new Panel("pnl_qz_tr", null, null);
        $pnl_ztr->add($pw1);
        $pnl_ztr->add($pw2);
        $pnl_ztr->add($pw3);


        ## Create main panel with submit button            ###############

        $btn = new Button("sub_qz", "Zoom", "sub_btn", "Click to zoom to location", 
                DF_LABEL_OVER, DF_BLOCK_NONE);
        $pnl_btn = new Panel("pnl_qz_sub", null, null);
        $pnl_btn->add($btn);

         $panel = new Panel("pnl_qz","Quick Zoom","title_pnl");
         $panel->add($pnl_zsel);
         $panel->add($pnl_zll);
         $panel->add($pnl_stc);
         $panel->add($pnl_zrg);
         $panel->add($pnl_ztr);
         $panel->add($pnl_btn);

        $panel->setValuesFromRequest($rq_array);
        $panel->setKeyWidgetPairs($kw);

        if ($kw['sel_qz']->getValue() == 'pnl_qz_ll')  {
            $kw['txt_qzlat']->setRequired(true);
            $kw['txt_qzlon']->setRequired(true);
        }

        return $panel;
    }
### ==========================================================================

    function sortMeridByText($a, $b) {
        ## $a and $b are arrays
        return ($a["text"] == $b["text"]) ? 0 : (($a["text"] < $b["text"]) ? -1 : 1);
    }
### ==========================================================================

    function & crViewPanel( &$rq_array, &$kw, &$viewNames) { ## Create View manager panel

        $panel = new Panel("pnl_vw", "View Manager", "title_pnl");

        ##  Action radio button selection panel ##

        $pnl_vw_rb = new Panel("pnl_vw_rb", null, null);
        $viewtoggle = new RadioButtonGroup("viewtoggle", "Select Action", "rb_pnl_sel", null,
                      DF_LABEL_TOP, DF_BLOCK_P, DF_LAYOUT_VERT);
        $viewtoggle->add("rad_view_new", "Save new view", "Save the map as a new view and retrieve it later");
        $viewtoggle->add("rad_view_upd", "Update view", "Save the map under an existing name and retrieve it later");
        $viewtoggle->add("rad_view_lod", "Load view", "Retrieve a map that was previously saved");
        $viewtoggle->add("rad_view_del", "Delete view", "Delete the map view in the list");
        $viewtoggle->setJSEvent("onclick");
        $viewtoggle->setJSMethod("set_viewmgr(this.id)");   # Special case here, only supply method name
           

        $pnl_vw_rb->add($viewtoggle);
        $panel->add($pnl_vw_rb);


        ##  New view panel  ##

        $pnl_vw_new = new Panel("pnl_vw_new", null, null);
        $txt_vw_new = new TextField("txt_vw_new", "Save View As", null,
                "New name for saving map view", DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 15, null);
        $txt_vw_new->setBlockClass("input1");

        $pnl_vw_new->add($txt_vw_new);
        $panel->add($pnl_vw_new);


        ## View select panel (for loading/saving/deleting existing views  ##

        $pnl_vw_selvw = new Panel("pnl_vw_selvw", null, null);
        $sel_view = new TextListBox("sel_view", "Select to load, save, or delete",
                    "sel_list", "Select layer to be queried or identified",
                    DF_LABEL_TOP, DF_BLOCK_P, DF_CHR, 1, null);
            $sel_view->setBlockClass("input1");

        foreach ($viewNames as $i => $u)
            $sel_view->add($i,$viewNames[$i]);

        $pnl_vw_selvw->add($sel_view);
        $panel->add($pnl_vw_selvw);

        ### ---  Submit  Button Panel --- ###

        $pnl_vw_sub = new Panel("pnl_vw_sub", null, null);
        $sub_vw = new Button("sub_vw", "Save View As", "sub_btn", "Click to save view", 
                DF_LABEL_OVER, DF_BLOCK_NONE);
        $pnl_vw_sub->add($sub_vw);
        $panel->add($pnl_vw_sub);

        
        $panel->setValuesFromRequest($rq_array);
        $panel->setKeyWidgetPairs($kw);
        return $panel;
    }

### ==========================================================================

    function & crPZoomPanel( &$rq_array, &$kw) {



        $row = new RowWidget("tbl_right_r", null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE);
        
        $pzg = new JSRadioButtonGroup("pzgroup", null, null, null, null, DF_BLOCK_NONE, DF_LAYOUT_HORIZ, "rb_up", "rb_down");
        $pzg->add("rb_zin", null, "Zoom in", "imagedir/zoomin_2.gif");
        $pzg->add("rb_zou", null, "Zoom out", "imagedir/zoomout_1.gif");
        $pzg->add("rb_pan", null, "Recenter map to cursor location", "imagedir/pan_1.gif");
        $pzg->add("rb_idn", null, "Identify feature at cursor location", "imagedir/thmIdOn.gif");

        $row->add($pzg);

        $row->add (new TextWidget("rb_filler",null,null, DF_BLOCK_NONE),"td_fill");

        $row->add(new Button("sub_fullext", "Full Map", "sub_btn_top", "Reset map to default view", DF_LABEL_OVER, DF_BLOCK_NONE), "td_btn_top");
        $row->add(new Button("sub_logout", "Logout", "sub_btn_top", "Logout and return to login page", DF_LABEL_OVER, DF_BLOCK_NONE), "td_btn_top");

        $tbl_rt = new TableWidget("tbl_right", null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE);
        $tbl_rt->add($row);


        $tbl_rt->setValuesFromRequest($rq_array);
        $tbl_rt->setKeyWidgetPairs($kw);

        return $tbl_rt;
    }
### ==========================================================================

    function layer_mapToWidget(&$map, &$wk) {
        ## Update the widget settings in the LayerPanel
        for ($i=0; $i<$map->numlayers; $i++) {
            $layerObj = $map->getLayer($i);

            ## Skip layers that don't require user control
            if ($layerObj->getMetaData("layertype") == "highlight") continue;

            ##echo "layer: " . $layerObj->name . "<br />\n";
            $widgetId = $layerObj->name . "_vi";
            if ($layerObj->status == MS_ON || $layerObj->status == MS_DEFAULT)
                $wk[$widgetId]->setValue(true);
            else
                $wk[$widgetId]->setValue(false);
        }
    }

### ==========================================================================

    function htmlWriteRefPanel(&$map) {

        ## collect extMap_xy =  (bot-lft, cen-lft, top-lft, top-cen, top-rgt, cen-rgt, bot-rgt, bot-cen)
        ## inv project extMap_xy -> extMap_ll
        ## 

        ## determine which ref map to use: 
        ## for each ref map, are all extMap_ll points contained in ref map lat/longs ?
        ## if so, break.  we have found the appropriate refence map

        ## Create array to hold xy points
        $xy_extMap = array();
        for ($i=0; $i< 8; $i++) array_push($xy_extMap, ms_newPointObj());

        ## Load array of 8 points around the extent boundary
        $xy_extMap[0]->setXY($map->extent->minx, $map->extent->miny);                                   ## bot left
        $xy_extMap[1]->setXY(  $map->extent->minx, ($map->extent->miny + $map->extent->maxy)/2  );      ## cen left
        $xy_extMap[2]->setXY($map->extent->minx, $map->extent->maxy);                                   ## top left
        $xy_extMap[3]->setXY(  ($map->extent->minx + $map->extent->maxx)/2, $map->extent->maxy  );      ## top center
        $xy_extMap[4]->setXY($map->extent->maxx, $map->extent->maxy);                                   ## top right
        $xy_extMap[5]->setXY(  $map->extent->maxx, ($map->extent->miny + $map->extent->maxy)/2  );      ## cen right
        $xy_extMap[6]->setXY($map->extent->maxx, $map->extent->miny);                                   ## bot right
        $xy_extMap[7]->setXY(  ($map->extent->minx + $map->extent->maxx)/2, $map->extent->miny  );      ## bot center


        ## Load meta data definitions for all possible refmaps
        $refmap_list = split(',', $map->getMetaData("refmaplist"));
        $refprj_list = split(',', $map->getMetaData("refprjlist"));
        $refxxy_list = split(',', $map->getMetaData("refxxylist"));
        $refxll_list = split(',', $map->getMetaData("refxlllist"));

        ## Use these proj definitions to transform the map extent points
        $prjIn = ms_newProjectionObj($map->getProjection());
        $prjOut= ms_newProjectionObj("proj=latlong");

        ## Default to first refmap in array
        ## This should also be the default refmap in the map file
        $ref_idx = 0;   
        $ptTst = ms_newPointObj();  ## A temp point used to test each map extent point
        for ($i=1; $i<count($refmap_list); $i++) {
            ## Get min/max long/lat of current ref map extents from metadata
            list($minx,$miny,$maxx,$maxy) = split(' ', $refxll_list[$i]);      ## lat long ref map extents
            $found = true;  ## Assume true and then try and prove false
            for ($j=0; $j<count($xy_extMap); $j++) {
                $ptTst->setXY(  $xy_extMap[$j]->x, $xy_extMap[$j]->y  );
                $ptTst->project($prjIn, $prjOut);
                if (  $ptTst->x < $minx || $ptTst->x > $maxx || $ptTst->y < $miny || $ptTst->y > $maxy ) {
                   $found = false;
                   break;
                }
            }
            if ($found) {
                $ref_idx = $i;
                break;
            }
        }
        list($minx,$miny,$maxx,$maxy) = split(' ', $refxxy_list[$ref_idx]);    ## xy ref map extents
        #$extRef = ms_newRectObj();
        #$extRef->setExtent($minx,$miny,$maxx,$maxy);
        $prjOut = ms_newProjectionObj(  preg_replace('/ /', ',', $refprj_list[$ref_idx]));  ## Replace space with comma

        ## Reproject map extent points from map projection to ref map projection
        for ($i=0; $i<count($xy_extMap); $i++) {
            $xy_extMap[$i]->project($prjIn, $prjOut);
            #echo "<!--  extMap (reprojected to extRef): " . $xy_extMap[$i]->x . " " . $xy_extMap[$i]->y . "  -->\n";
        }

        ## Generate unique file name for the reference map, which will be created by using
        ## a predefined image and superimposing the index location.
        $jdate = gettimeofday();
        $imgname = 'rf' . $jdate['sec'].$jdate['usec'] . '.png';
        $pngfile = $_SERVER["DOCUMENT_ROOT"]
                 . "/" . $map->getMetaData("refmappath")
                 . $refmap_list[$ref_idx];

        $img = ImageCreateFromPNG($pngfile);
        $path = $map->web->imagepath . $imgname;
        $url = $map->web->imageurl . $imgname;

        $pminx = 99999;  $pmaxx = -99999; $pminy = 99999;  $pmaxy = -99999;
        $xypts = array();
        $xsc = $map->reference->width/($maxx - $minx);
        $ysc = $map->reference->height/($maxy - $miny);

        for ($i=0; $i<count($xy_extMap); $i++) {
            $x = (int)(.5 + ($xy_extMap[$i]->x - $minx) * $xsc );
            $y = (int)(.5 + $map->reference->height - ($xy_extMap[$i]->y - $miny) * $ysc );

            $pminx = $pminx < $x ? $pminx : $x;
            $pmaxx = $pmaxx > $x ? $pmaxx : $x;
            $pminy = $pminy < $y ? $pminy : $y;
            $pmaxy = $pmaxy > $y ? $pmaxy : $y;

            array_push($xypts, $x);
            array_push($xypts, $y);
        }
        $red   = ImageColorAllocate($img, 0xff, 0x00, 0x00);
        $szMarker = 7;
        if ($pmaxx - $pminx < $szMarker || $pmaxy - $pminy < $szMarker) {
            $xc = ($pmaxx + $pminx)/2;
            $yc = ($pmaxy + $pminy)/2;
            $x1 = $xc-(int)$szMarker/2 > 0 ? $xc-(int)$szMarker/2 : 0;
            $x2 = $xc+(int)$szMarker/2 < $map->reference->width ? $xc+(int)$szMarker/2 : $map->reference->width;
            $y1 = $yc-(int)$szMarker/2 > 0 ? $yc-(int)$szMarker/2 : 0;
            $y2 = $yc+(int)$szMarker/2 < $map->reference->height ? $yc+(int)$szMarker/2 : $map->reference->height;
            ImageLine($img, $x1, $yc, $x2, $yc, $red);
            ImageLine($img, $xc, $y1, $xc, $y2, $red);
        }
        else
            ImagePolygon ($img, $xypts, count($xypts)/2, $red);

        ImagePNG($img, $path);

        echo "<div id=\"pnl_rf\">\n";
        echo "    <h3 class=\"title_pnl\">Index Map</h3>\n";
        echo "    <div id=\"pnl_rf_map\">\n";
        printf("    <input type=\"hidden\" name=\"REFMAPXSIZE\" value=\"%d\" />\n", $map->reference->width);
        printf("    <input type=\"hidden\" name=\"REFMAPYSIZE\" value=\"%d\" />\n", $map->reference->height);

        ## disable input from the reference map
        #echo "    <input type=image src=\"$url\" border=1 name=\"refmap\""
        #    . " height=\"" . $map->reference->height . "\" width=\"" . $map->reference->width . "\""
        #    . " style='cursor : crosshair;' />\n";
        echo "    <img src=\"$url\""
            . " alt=\"index map\""
            . " height=\"" . $map->reference->height . "\" width=\"" . $map->reference->width . "\" />\n";
        echo "    </div>\n";
        echo "</div>         <!--         pnl_rf               -->\n";
    }
        
### ==========================================================================

    function htmlWriteQSetPanel(&$qset, &$map) {
        echo "<div id=\"pnl_qs\">\n";
        echo "    <h3 class=\"title_pnl\">Query Results</h3>\n";
        echo "    <div id=\"pnl_qs_data\">\n";
        $qset->htmlWrite($map);
        echo "    </div>     <!--         pnl_qset_data         -->\n";
        echo "</div>         <!--         pnl_qset              -->\n";
    }
### ==========================================================================
    function htmlWrite_jstabs(&$list) {
        $firstTime = true;
        $comma = "";
        foreach ($list as $k => $v) {
            echo $comma . "\"" . $k . "\"";
            $comma = ",";
        }
    }

    function htmlWrite_jspnls(&$list) {
        foreach ($list as $k => $v)
            echo "        pnlList['" . $k . "'] = '" . $v . "';\n";
    }

    function getLastPanel(&$rqArray) {
        if ( isset($rqArray['selected_pnl'])  ) return $rqArray['selected_pnl'];
        else return 'pnl_qy';
    }

    function getLastQzPanel(&$rqArray) {
        if ( isset($rqArray['sel_qz_pnl'])  ) return $rqArray['sel_qz_pnl'];
        else return 'zbyLL';
    }

    function getLastVwPanel(&$rqArray) {
        if ( isset($rqArray['selected_vw_pnl'])  ) return $rqArray['selected_vw_pnl'];
        else return 'viewtoggle_rad_view_new';
    }
### ==========================================================================

    function drawScaleBar(&$map) {

        $img = $map->drawScaleBar();
        $url = $img->saveWebImage();
        echo "<img src=\"" . $url . "\" width=\"" . $img->width
                . "\" height=\"" . $img->height . "\" alt=\"scale bar\" />";
    }

### ==========================================================================

    function drawMainMap2(&$map, &$querySet) {
        ## Generate the map image. This version of function
        ## puts image reference between <style> tags

        ## Look for special data strings that scale or min dist value to be passed
        #for ($i=0; $i<$map->numlayers; $i++) {
        #    $layerObj = &$map->getLayer($i);
        #    if ( preg_match('/shotpoints *\(([a-zA-Z0-9_]*)\)/', $layerObj->data, $datastr) ) {
        #        $value = $layerObj->getMetaData("sp_interval");
        #        $value = $value ? $value : 10;
        #        ##                         $1                  $2             $3
        #        $rstr = '$1 ' . $value . ' $3';
        #        $newdata = preg_replace('/(^.* *shotpoints *\()([a-zA-Z0-9_]*)(\).*$)/', "$rstr", $layerObj->data);
        #        $layerObj->set("data", $newdata);
        #    }
        #}

        if ($querySet->isActivated()) {
            $img = $map->drawQuery();
         ## #$rtn=$map->savequery('/data1/www/data/tmpdata/deleteme.qy');
         ## #if ($rtn == MS_TRUE) echo "<!--   savequery, MS_TRUE  rtn=" . $rtn . "   -->\n";
         ## #else echo "<!--  savequery, MS_FALSE rtn=" . $rtn . "   -->\n";
        }
        else {
         ## #$rtn=$map->loadquery('/data1/www/data/tmpdata/deleteme.qy');
         ## #if ($rtn == MS_TRUE) echo "<!--   loadquery, MS_TRUE  rtn=" . $rtn . "   -->\n";
         ## #else echo "<!--  loadquery, MS_FALSE rtn=" . $rtn . "   -->\n";
 
            $img = $map->draw();
        }
        #################################################################################
        ######    Image map experiment
        #$img = $map->drawQuery();
        $layerObj = $map->getLayerByName('cseis');
        #error_log("rtn=" . $rtn . ", error=" . ($rtn==MS_FAILURE) . ", success=" .($rtn==MS_SUCCESS) );
 
        $nresults = $layerObj->getNumResults();
        #error_log("num results=" . $nresults);
        for ($i=0; $i<$nresults; $i++) {
            $resultObj = $layerObj->getResult($i);
            error_log("shapeindex=".$resultObj->shapeindex . ", classindex=" . $resultObj->classindex);
        }
 
        #################################################################################
    ###  - - - - - -  - - - - - - - - - - - 
        $querySet->drawRect($map, $img);
        $url = $img->saveWebImage();
        $map->setMetaData("imagemapurl", $url);
 
        echo "#mapdiv {\n";
        echo "            height: " . $map->height . "px;\n";
        echo "            width: " . $map->width . "px;\n";
        echo "            border: 1px solid black;\n";
        echo "            background: transparent url(\"" . $url . "\") no-repeat top left;\n";
        echo "            left: 0;\n";
        echo "            top: 0;\n";
        echo "            position: relative;\n";
        echo "        }\n";
    }

### ==========================================================================

    function drawMainMap(&$map) {

        $activateMouseovers = (strtoupper($map->getMetaData("mouseovers")) == "Y" ? true : false);

        echo "<input type=\"hidden\" name=\"projstr\" value=\"" . $map->getProjection() . "\" />\n";
        printf("<input type=\"hidden\" name=\"extents\" value=\"%f %f %f %f\" />\n",
                        $map->extent->minx, $map->extent->miny,
                        $map->extent->maxx, $map->extent->maxy);

        printf("<input type=\"hidden\" name=\"imagewidth\" value=\"%d\" />\n", $map->width);
        printf("<input type=\"hidden\" name=\"imageheight\" value=\"%d\" />\n", $map->height);
        echo "<div id=\"mapdiv\">\n";

        ##---------------------------------------------------------------------
        ## Create client-side image map for mouseovers
        $lyMouse = $map->getLayerByName("cseis");

        ## Only generate image map if the layer will be shown and is visible
        $doIMap = false;
        if ( $activateMouseovers
            &&  ($lyMouse->maxscale == -1 || $lyMouse->maxscale > $map->scale)
            &&  ($lyMouse->labelmaxscale == -1 || $lyMouse->labelmaxscale > $map->scale)
            &&  ($lyMouse->status == MS_ON || $lyMouse->status == MS_DEFAULT) ) {
            $doIMap = true;

            $mapImg = ms_newMapObj("mouseovers.map");
            $mapImg->setSize($map->width, $map->height);
            $mapImg->setExtent( $map->extent->minx, $map->extent->miny,
                                $map->extent->maxx, $map->extent->maxy );
            $mapImg->setProjection($map->getProjection());

            ## Turn off non-relevant layers and set connection type for that layer
            for ($i=0; $i<$mapImg->numlayers; $i++) {
                $ly = $mapImg->getLayer($i);
                if ($ly->name == $lyMouse->name) {
                    $ly->set("status", MS_DEFAULT);
                    $lyMouse = $map->getLayerByName($ly->name);
                    $ly->set("connection", $lyMouse->connection);
                    $ly->set("connectiontype", $lyMouse->connectiontype);
                }
                else {
                    $ly->set("status", MS_DELETE);
                }

            }

            $imgM = $mapImg->draw();
        }
        ##---------------------------------------------------------------------

        echo "    <input type=\"image\" src=\"" . $map->getMetaData("transp_map") . "\""
                  . " name=\"mainmap\" id=\"mainmap\""
                  . ($doIMap ? " usemap=\"#" . $mapImg->outputformat->getOption("MAPNAME") . "\"" : "")
                  . " style='cursor : crosshair;' />\n";

        ##
        ##  output image map
        if ($doIMap) {
            echo "<map name=\"" . $mapImg->outputformat->getOption("MAPNAME") . "\""
               . " width=\"" . $mapImg->width . "\" height=\"" . $mapImg->height . "\""
               . ">\n"; 

            $rtn = $imgM->saveImage("", $mapImg);
            echo "</map>\n";
        }
        ##
        echo "</div>\n";
    }

### ==========================================================================

    function px2wRect ( &$rectIn, &$rectOut, &$map, &$curExt ) {
        // compute world coordinates of rectangle from image coordinates
        $wMinx   = $curExt->minx + $rectIn->minx
                    * ($curExt->maxx - $curExt->minx)/$map->width;

        $wMaxx   = $curExt->minx + $rectIn->maxx
                    * ($curExt->maxx - $curExt->minx)/$map->width;

        ## Note, image y coordinate is 0 at top and maxy at bottom
        ##       and map y increases from bottom to top.
        
        $wMiny   = $curExt->miny + ($map->height - $rectIn->miny)
                    * ($curExt->maxy - $curExt->miny)/$map->height;

        $wMaxy   = $curExt->miny + ($map->height - $rectIn->maxy)
                    * ($curExt->maxy - $curExt->miny)/$map->height;

        $tmp =      ($curExt->maxy - $curExt->miny)/$map->height;
        
        $rectOut->setExtent($wMinx, $wMiny, $wMaxx, $wMaxy);
    }

## =====================================================================

    function px2wPoint (&$pointIn, &$pointOut, &$map, &$curExt) {
        // compute world coordinates of clicked point from image coordinates
        $xw      = $curExt->minx + $pointIn->x
                    * ($curExt->maxx - $curExt->minx)/$map->width;

        $yw      = $curExt->miny
                    + ($map->height - $pointIn->y)
                    * ($curExt->maxy - $curExt->miny)/$map->height;
        
        $pointOut->setXY($xw, $yw);
    }

### ==========================================================================
    function setLegendIcons(&$map, &$kw) {


        for ($i=0; $i<$map->numlayers; $i++) {
            $lyObj = $map->getLayer($i);
            if ($lyObj->getMetaData("no_icon") == "Y") continue;     ## This layer doesn't need an icon
            for ($j=0; $j<$lyObj->numclasses; $j++) {
                $iwName = "lgimg_".$i."_".$j;
                if (array_key_exists($iwName, $kw)) {
                    $clObj = $lyObj->getClass($j);
                    $img = $clObj->createLegendIcon($map->legend->keysizex, $map->legend->keysizey);
                    $url = $img->saveWebImage();
                    $img->free();
                    $kw[$iwName]->setSrc($url, $map->legend->keysizex, $map->legend->keysizey, $kw[$iwName]->title);
                }
                else
                    echo "Widget not found: ".$iwName."<br />\n";
            }
        }
    }
### ==========================================================================
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>GIServe Web Mapping Application</title>
<style type="text/css" media="all">@import "position.css";</style>
<style type="text/css" media="all">
        <?php drawMainMap2($gbMapmgr->getMapObject(), $gbQuerySet) ?>
</style>
<meta http-equiv="Content-type" content="text/html; charset=iso-8859-1" />
<meta name="MSSmartTagsPreventParsing" content="true" />
    <script src="counties.js" type="text/javascript"></script>
    <script src="twnrng.js" type="text/javascript"></script>
    <script type="text/javascript">
    <!--
        var NS4DOM = document.layers ? true:false;           // netscape 4
        var IEDOM = document.all ? true:false;               // ie4+
        var W3CDOM = document.getElementById && !(IEDOM) ? true:false;   // netscape 6
        var imgCollapsed = new Image(11,11); imgCollapsed.src = "imagedir/collapsed_4.gif";
        var imgExpanded = new Image(11,11); imgExpanded.src = "imagedir/expanded_4.gif";
        var iWidth=<?php echo $gbMap->width;?>;
        var iHeight=<?php echo $gbMap->height;?>;
        var newWindow;
    //
        var hlImgFiles= new Array(<?php $gbQuerySet->htmlWritePreload() ?>);
        var blankMap = new Image(<?php echo $gbMap->width . "," . $gbMap->height;?>);
            blankMap.src = "<?php echo $gbMap->getMetaData("transp_map")?>";
        var hlImages = new Array();
        for (i=0; i<hlImgFiles.length; i++) {
            hlImages[i] = new Image(<?php echo $gbMap->width . "," . $gbMap->height;?>);
            hlImages[i].src = 
                '<?php echo $gbMap->web->imageurl ?>' + hlImgFiles[i] +
                '.<?php echo $gbMap->outputformat->extension ?>';
        }

        //var tabList = new Array("pnl_qy", "pnl_qs","pnl_qz", "pnl_in", "pnl_ly", "pnl_vw", "pnl_rf");
        var pnlList = new Array("pnl_qy", "pnl_qs","pnl_qz", "pnl_in", "pnl_ly", "pnl_vw", "pnl_rf");
        var spnlList = new Array();
        spnlList['pnl_qy'] = new Array('pnl_qy_seis','pnl_qy_soil'); //,'pnl_qy_river');
        spnlList['pnl_qs'] = new Array();
        spnlList['pnl_qz'] = new Array('pnl_qz_ll','pnl_qz_st','pnl_qz_rg','pnl_qz_tr');
        spnlList['pnl_vw'] = new Array('pnl_vw_new','pnl_vw_selvw');
        spnlList['pnl_in'] = new Array();
        spnlList['pnl_ly'] = new Array();
        spnlList['pnl_rf'] = new Array();
        var btnList = new Array("rb_zin", "rb_zou", "rb_pan", "rb_idn");
        var last_btn = btnList[0];
        var selected_pnl = pnlList[0];


        var defSpnl = new Array();   // Default subpanels
        defSpnl['pnl_qy'] = 'pnl_qy_seis';
        defSpnl['pnl_qz'] = 'pnl_qz_ll';
        defSpnl['pnl_vw'] = 'pnl_vw_new';

        // View manager
        var vw_rbList = new Array('viewtoggle_rad_view_new','viewtoggle_rad_view_upd','viewtoggle_rad_view_lod','viewtoggle_rad_view_del');
        var vw_rbValList = new Array()
            vw_rbValList['viewtoggle_rad_view_new'] = "Save As";
            vw_rbValList['viewtoggle_rad_view_upd'] = "Save";
            vw_rbValList['viewtoggle_rad_view_lod'] = "Load";
            vw_rbValList['viewtoggle_rad_view_del'] = "Delete";
        var sel_vw_pnl = vw_rbList[0];
        var qzTrgList = new Array('zbyLL', 'zbyState', 'zbyReg', 'zbyTwrg');
        var qzList = new Array();
        qzList['zbyLL'] = 'pnl_qz_ll';
        qzList['zbyState'] = 'pnl_qz_state';
        qzList['zbyReg'] = 'pnl_qz_reg';
        qzList['zbyTwrg'] = 'pnl_qz_twrg';
        var sel_qz_pnl = qzTrgList[0];


        function initpage() {
            var lastpanel = '<?php echo getLastPanel($_POST)?>';
            if (!lastpanel) lastpanel = pnlList[0];
            selPanel(lastpanel);
            btnOnclick('<?php echo $gbKWPairs['pzgroup']->getValue();?>');
            set_viewmgr('<?php echo getLastVwPanel($_POST)?>');
            //set_qzpanel('<?php echo getLastQzPanel($_POST)?>');

            selSubPanel('pnl_qy','sel_qy');
            selSubPanel('pnl_qz','sel_qz');
            //selSubPanel('pnl_vw','sel_vw_new');
            //rbSubPanel(vw_rbList);
            crLayerNode();
            twnGrp = new TwnRngObj(0, "txt_qztwn_out","txt_qztwn","txt_qztwn_msg","ch_qztwn_h","ch_qztwn_n","ch_qztwn_s","sel_qzmer");
            rngGrp = new TwnRngObj(1, "txt_qzrng_out","txt_qzrng","txt_qzrng_msg","ch_qzrng_h","ch_qzrng_e","ch_qzrng_w","sel_qzmer");
            twnGrp.init("<?php echo $gbKWPairs['sel_qzmer']->getValue();?>");
            rngGrp.init("<?php echo $gbKWPairs['sel_qzmer']->getValue();?>");
        }

        function loadCounty(cntyListBox, stateListBox) {
            var cntyArray = counties [ stateListBox.options[ stateListBox.selectedIndex ].value ];
            var i;
            cntyListBox.options.length = 0;
            cntyListBox.options[0] = new Option('- Whole State -', 'all', false, false);
            for (i=0; i<cntyArray.length; i++) {
                var pair = cntyArray[i].split(',',2);
                cntyListBox.options[i+1] = new Option(pair[1], pair[0], false, false);
            }
        }

        function updTRInfo() {
            twnGrp.update();
            rngGrp.update();
        }

        function imapSelect(area, mcode) {
            getObject("sel_qzmer").value = mcode;
            twnGrp.update();
            rngGrp.update();
        }
        
        function imapTitle(area, mcode) {
            var obj = getObject("sel_qzmer");
            var i=0;
            var found = false;
            while (i<obj.options.length && !found) {
                if ( obj.options[i].value == mcode ) {
                    area.title = obj.options[i].text;
                    return;
                }
                i++;
            }
            area.title=null;
        }

        // ========    LayerNode class         ============================================

        function expandCollapse(lyname) {
            //if (lyname != "reg" && lyname != "mis") return;
            rootNode.toggleState(lyname);
        }

        LayerNode.ImgExpanded  = new Image(11,11); LayerNode.ImgExpanded.src = "imagedir/expanded_4.gif";
        LayerNode.ImgCollapsed = new Image(11,11); LayerNode.ImgCollapsed.src = "imagedir/collapsed_4.gif";
        function LayerNode(name) {
            this.name = name;
            this.isOpen = true;
            this.children = null;
            this.imgName = null;
        }

        LayerNode.prototype.addChild = function (child) {
            if (!this.children)
                this.children = new Array();
            this.children.push(child);
        }

        LayerNode.prototype.init = function() {
            if (!this.children) return;
            var i;
            for (i=0; i<this.children.length; i++) {
                this.children[i].showChildren();
            }
        }

        LayerNode.prototype.setImageName = function(img) {  this.imgName = img; }

        LayerNode.prototype.getState = function (name) {
            var node = this.findNode(name);
            return node.isOpen;
        }

        LayerNode.prototype.getAllStates = function() {
            if (!this.children) return "";
            var i;
            var states = this.name +"="+ (this.isOpen ? 1:0);
            for (i=0; i<this.children.length; i++) {
                var st = this.children[i].getAllStates();
                if (st != "") states += ","+st;
            }
            return states;
        }

        LayerNode.prototype.toggleState = function(name) {
            var node = this.findNode(name);
            if (!node) return;
            if (node.isOpen) { node.hideChildren(); node.isOpen = false; }
            else { node.showChildren(); node.isOpen = true; }
        }

        LayerNode.prototype.setState = function (name, isOpen) {
            // This just sets the isOpen property of the specified node
            // This is just for bookkeeping and does not actually do
            // anything. The setVisibleState() method shows or hides the node
            // 
            var node = this.findNode(name);
            node.isOpen = isOpen;

            // Now set the css or html property to show or hide the node
            // The show/hide action is recursive
            if (isOpen) node.showChildren();
            else node.hideChildren();
        }

        LayerNode.prototype.setAllStates = function(states) {
            if (!states || states == "") { this.init(); return; }
            var stArr = states.split(",");
            var i;
            for(i=0; i<stArr.length; i++) {
                var words = stArr[i].split("=");
                if (words[0] != this.name) this.setState(words[0], parseInt(words[1]));
            }
        }

        LayerNode.prototype.showChildren = function () {
            if (!this.children)
                return;
            var i;
            for (i=0; i<this.children.length; i++) {
                setDisplayProp(this.children[i].name, null);
                if (this.children[i].isOpen)
                    this.children[i].showChildren();  // recursive call
            }
            if (this.imgName) {
                var obj = getObject(this.imgName);
                obj.src = LayerNode.ImgExpanded.src;   
                obj.title = "Click to collapse";
            }
        }

        LayerNode.prototype.hideChildren = function () {
            var i;
            if (!this.children)
                return;
            for (i=0; i<this.children.length; i++) {
                setDisplayProp(this.children[i].name, "none");
                this.children[i].hideChildren();  // recursive call
            }
            if (this.imgName) {
                var obj = getObject(this.imgName);
                obj.src = LayerNode.ImgCollapsed.src;
                obj.title = "Click to expand";
            }
        }

        LayerNode.prototype.findNode = function (name) { return this.find(name); }

        LayerNode.prototype.find = function (name) {
            var i;
            if (!this.children)
                return null;
            for (i=0; i<this.children.length; i++) {
                if (this.children[i].name == name)
                    return this.children[i];
                else {
                    var child = this.children[i].find(name);
                    if (child) return child;
                }
            }
            return null;
        }

        // ========   End  LayerNode class         ============================================
        function crLayerNode() {
        <?php $tree->writeJSString(); ?>
            rootNode.setAllStates( <?php echo (isset($_POST["layer_nodes"]) ? "\"".$_POST["layer_nodes"]."\"":"null");?> );
        }

        function submitForm() {
            setHiddenFld('selected_pnl', selected_pnl);
            setHiddenFld('selected_vw_pnl', sel_vw_pnl);
            setHiddenFld('selected_qz_pnl', sel_qz_pnl);
            setHiddenFld('pzgroup', last_btn);
            var states = rootNode.getAllStates();
            setHiddenFld('layer_nodes', states);

            var val = getObject('sel_qy').value;
            if (val == "pnl_qy_river")
                setHiddenFld("activelayer","streams");
            else if (val == "pnl_qy_soil")
                setHiddenFld("activelayer","samples");
            else
                setHiddenFld("activelayer","cseis");
//alert("Hidden field, activelayer="+ getObject("activelayer").value);
        }

        function selPanel(id) {
            // Select panel from tab id
            if (! spnlList[id]) return;
            for (var i=0; i<pnlList.length; i++) {
                var obj_a = getObject(pnlList[i].replace(/pnl_/,"tab_"));
                var obj_li = getObject(pnlList[i].replace(/pnl_/,"litab_"));

                if (id == pnlList[i]) {
                    displayIt(pnlList[i], true);
                    selected_pnl = id;
                    obj_a.style.backgroundImage = "url(imagedir/blue_right_on.gif)";
                    obj_li.style.backgroundImage = "url(imagedir/blue_left_on.gif)";
                }
                else {
                    displayIt(pnlList[i], false);
                    obj_a.style.backgroundImage = "url(imagedir/blue_right.gif)";
                    obj_li.style.backgroundImage = "url(imagedir/blue_left.gif)";
                }
            }
        }

        function selSubPanel(idPnl, id) {
            var selbox;
            selbox = getObject(id);
            if (!selbox) return;
            for (var i=0; i<spnlList[idPnl].length; i++) {
                if (selbox.value == spnlList[idPnl][i]) {
                    displayIt(spnlList[idPnl][i], true);
                }
                else displayIt(spnlList[idPnl][i], false);
            }
        }

        function btnOnclick(id) {
            for (var i=0; i<btnList.length; i++) {
                if (id == btnList[i]) {  
                    setButtonState(btnList[i], true);
                    last_btn = id;
                }
                else setButtonState(btnList[i], false);
            }
        }

        function setButtonState(id, state) {
            var newClass;
            if (state) newClass = 'rb_down';
            else newClass = 'rb_up';

            if (IEDOM) { 
                elem = document.getElementsByName(id);
                elem[0].className = newClass;
            }
            else if (W3CDOM) {
                var object = getObject(id);
                object.className = newClass;
            }
        }

        function setHiddenFld(id, value) {
            if (IEDOM) {
                elem = document.getElementsByName(id);
                elem[0].value = value;
            }
            else if (W3CDOM) {
                obj = getObject(id);
                obj.value = value;
            }
        }

        function set_viewmgr(id) {
            if (id == 'viewtoggle_rad_view_new') {
                displayIt('pnl_vw_selvw', false);
                displayIt('pnl_vw_new', true);
            }
            else {
                displayIt('pnl_vw_new', false);
                displayIt('pnl_vw_selvw', true);
            }
            obj = getObject('sub_vw');
            obj.value = vw_rbValList[id];
            sel_vw_pnl = id;
        }

        function hideIt(id) {
            var object = getObject(id);
            if (NS4DOM) object.visibility = "hide";
            else if (IEDOM || W3CDOM) object.style.visibility = "hidden";
        }

        function showIt(id) {
            var object = getObject(id);
            if (NS4DOM) object.visibility = "show";
            else if (IEDOM || W3CDOM) object.style.visibility = "visible";
        }

        function getObject(id) {
            var ref;
            if (NS4DOM) ref="document."+id;
            else if (IEDOM) {
                elem = document.getElementsByName(id);
                return elem[0];
            }
            else if (W3CDOM) ref = "document.getElementById('"+id+"')";
            var object=eval(ref);
            return object;
        }

        function displayIt(id, value) {
            var object = getObject(id);
            if (NS4DOM) {
               if (value) object.display = "block";
               else object.display = "none";
            }
            else if (IEDOM || W3CDOM) {
               if (value) object.style.display = "block";
               else object.style.display = "none";
            }
        }

        function isDisplayed(id) {
            var object = getObject(id);
            if (NS4DOM) {
               if (object.display === "block") return true;
               else return false;
            }
            else if (IEDOM || W3CDOM) {
               if (object.style.display == "block") return true;
               else return false;
            }
        }

        function setDisplayProp(id, value) {
            var object = getObject(id);
            if (NS4DOM) object.display = value;
            else if (W3CDOM) object.style.display = value;
            else if (IEDOM) object.style.display = value?value:"block";
        }

        function setvalue(id, value) {
            if (IEDOM) {
                elem = document.getElementsByName(id);
                elem[0].value = value;
            }
            else if (W3CDOM) {
                obj = getObject(id);
                obj.value = value;
            }
        }

        function getvalue(id) {
            if (IEDOM) {
                elem = document.getElementsByName(id);
                return elem[0].value;
            }
            else if (W3CDOM) {
                obj = getObject(id);
                return obj.value;
            }
        }

        function opDetailWindow(tblname, uid) {
            var windowFeatures;
            var xwd = 400;
            var yht = 500;
            var scx = 70;
            var scy = 140;
            if (W3CDOM) {
               windowFeatures = "width=" + xwd + ",height=" + yht;
               windowFeatures += ",resizable";
               windowFeatures += ",scrollbars";
               windowFeatures += ",screenX=" + scx + ",screenY=" + scy;
            }
            else {
               windowFeatures = "directories=0,location=0,menubar=0,resizeable=1,scrollbars=1,status=1,toolbar=0";
               windowFeatures += ",width=" + xwd + ",height=" + yht;

               //winTop =  screen.height/2 - 125;
               //winLeft = screen.width/2 - 125;
               //windowFeatures = windowFeatures + ",left=" + winLeft + ",top=" + winTop;
            }
            //
            var qstr = encodeURI("?tblname=" + tblname + "&uid=" + uid);
            var url = 'giserv_details.<?php echo preg_replace('/^.*\.(\w+)$/', '\1', $_SERVER['PHP_SELF']) ?>' + qstr;
            var name = 'detailWindow';
            if (typeof(newWindow) != "undefined") newWindow.focus();
            newWindow = window.open(url,name,windowFeatures);
        }

        function swapToHlt(id, fid) {
            obj = getObject(id);
            obj.src = hlImages[fid].src;
        }

        function swapToBase(id) {
            obj = getObject(id);
            obj.src = blankMap.src;
        }

        function windowUnload() {
           if (typeof(newWindow) != "undefined" && !newWindow.closed)
               newWindow.close();
        }

        // -->
    </script>
</head>

<body class="mainstyle" onload="initpage();" onunload="windowUnload()">
<form method="post" id="mapserv" action="<?php echo $_SERVER["PHP_SELF"]?>" onsubmit="submitForm();">
    <!-- ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ -->
<table id="tbl_main">
<tr><td valign="top">
    <table id="tbl_map" cellpadding="0" cellspacing="0">
        <tr>
            <td align="right"> <input type="image" src="imagedir/triangle_nw.gif" name="pan_nw" id="pan_nw" alt="pan_nw" />  </td>
            <td align="center"> <input type="image" src="imagedir/triangle_n.gif" name="pan_n" id="pan_n" alt="pan_n" />  </td>
            <td align="left"> <input type="image" src="imagedir/triangle_ne.gif" name="pan_ne" id="pan_ne" alt="pan_ne" />  </td>
        </tr>
        <tr>
            <td> <input type="image" src="imagedir/triangle_w.gif" name="pan_w" id="pan_w" alt="pan_w" />  </td>
            <td>
                <?php drawMainMap($gbMapmgr->getMapObject()) ?>
            </td>
            <td> <input type="image" src="imagedir/triangle_e.gif" name="pan_e" id="pan_e" alt="pan_e" />  </td>
        </tr>
        <tr>
            <td align="right"> <input type="image" src="imagedir/triangle_sw.gif" name="pan_sw" id="pan_sw" alt="pan_sw" />  </td>
            <td align="center"> <input type="image" src="imagedir/triangle_s.gif" name="pan_s" id="pan_s" alt="pan_s" />  </td>
            <td align="left"> <input type="image" src="imagedir/triangle_se.gif" name="pan_se" id="pan_se" alt="pan_se" />  </td>
        </tr>
        <tr><td align="center" colspan="3"> <?php drawScaleBar($gbMapmgr->getMapObject()) ?></td></tr>
    </table>
    <p class="copyright">
        Copyright &#064;2005 DataFlow Design, Inc. All rights reserved.
    </p>
</td>
<td valign="top">
    <!-- ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ -->
    <!-- ++++++++++ Navigation Buttons     ++++++++++++++++++++++++++++++++ -->
<?php
    $pzPanel->htmlWrite();
?>

    <!-- ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ -->
    <!-- ++++++++  Control panels goes here          +++++++++++++++++++++++ -->
    <div id="pnl_base">
        <div id="pnl_tabs">
            <ul>
            <li id="litab_qy"><a id="tab_qy" class="tabs" href="#" onclick="selPanel('pnl_qy');return false;" title="Run a query">Query</a></li>
            <li id="litab_qs"><a id="tab_qs"  class="tabs" href="#" onclick="selPanel('pnl_qs');return false;" title="Show query results">Results</a></li>
            <li id="litab_qz"><a id="tab_qz"  class="tabs" href="#" onclick="selPanel('pnl_qz');return false;" title="Zoom to a pre-defined view">Zoom</a></li>
            <li id="litab_ly"><a id="tab_ly"  class="tabs" href="#" onclick="selPanel('pnl_ly');return false;" title="Turn on/off layers, set query layer">Layers</a></li>
            <li id="litab_vw"><a id="tab_vw"  class="tabs" href="#" onclick="selPanel('pnl_vw');return false;" title="Manage user defined map views">Views</a></li>
            <li id="litab_rf"><a id="tab_rf" class="tabs" href="#" onclick="selPanel('pnl_rf');return false;" title="Display index map">Index</a></li>
            <li id="litab_in"><a id="tab_in" class="tabs" href="#" onclick="selPanel('pnl_in');return false;" title="Display map information">Info</a></li>
            </ul>
        </div>      <!--         End Tab Panel         -->
        <!-- ++++++++             Begin Conrtol Panels              +++++++++++ -->
<?php
    ###dbgPrintKW($gbKWPairs);   ## <<<<<<   Debug print
    $qyPanel->htmlWrite();
    $qzPanel->htmlWrite();
    setLegendIcons($gbMap, $gbKWPairs);
    $lyPanel->htmlWrite();
    $vwPanel->htmlWrite();
    htmlWriteRefPanel($gbMap);
    htmlWriteQSetPanel($gbQuerySet, $gbMap);
###How about error messages?
?>
<div id="pnl_in">
    <h3 class="title_pnl">Info</h3>
    <div id="pnl_in_data">
<?php
    #foreach ($gbKWPairs as $i => $k) {
    #    echo "<tr><td>" . $gbKWPairs[$i]->getName() . "</td><td>"
    #       . $gbKWPairs[$i]->getValue() . "</td></tr>\n";
    #}
?>
    <?php
          echo $gbMap->extent->minx ." " . $gbMap->extent->miny
             . " " . $gbMap->extent->maxx ." " . $gbMap->extent->maxy
             ."<br />\n";
          echo "scale= ".round($gbMap->scale,0)."<br />\n";
          echo $gbMap->getProjection() . "<br />\n";
          for ($i=0; $i<count($gbErrorMsgs); $i++)
              echo "<br /><span class=\"err_invalid\">" . $gbErrorMsgs[$i] . "</span>\n";

          ##dbgPrintKW($gbKWPairs);   ## <<<<<<   Debug print
          foreach ($_POST as $i=>$u)
              echo $i . "=". $_POST[$i] . "<br />\n";

          echo "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx<br />\n";
          $wlist = $qyPanel->getWidgetList();
          for ($i=0; $i<count($wlist); $i++) {
              if (
                     ereg('pnl_qy_', $wlist[$i]->name)
                  && $wlist[$i]->name != "pnl_qy_rb"
                  && $wlist[$i]->name != "pnl_qy_sub"
                 ) {
                  echo $wlist[$i]->name . "<br />\n";
              }
          }
    ?>
    </div>
</div>
        <!-- ++++++++              End Control Panels               +++++++++++ -->
<div id="pnl_err">
    <h3 class="title_pnl">Messages</h3>
    <p id="pnl_err_msg">
        <?php
            for ($i=0; $i<count($gbErrorMsgs); $i++)
                echo "<br /><span id=\"pnl_errormsg\" class=\"err_invalid\">" . $gbErrorMsgs[$i] . "</span>\n";
        ?>
    </p>
</div>
    </div>
</td> </tr> </table>
<div id="div_selpnls">
    <input type="hidden" name="selected_pnl" id="selected_pnl" value="" />
    <input type="hidden" name="selected_vw_pnl" id="selected_vw_pnl" value="" />
    <input type="hidden" name="selected_qz_pnl" id="selected_qz_pnl" value="" />
    <input type="hidden" name="activelayer" id="activelayer" value="" />
    <input type="hidden" name="imgxy" value="" />
    <input type="hidden" name="imgbox" value="-1 -1 -1 -1" />
    <input type="hidden" name="layer_nodes" id="layer_nodes" value="" />
</div>
</form>
</body>
    <script src="zoombox.js" type="text/javascript"></script>
</html>
