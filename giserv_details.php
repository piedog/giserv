<?php
######################################################################################
##  $Id: giserv_details.php,v 1.1.1.1 2007/04/24 02:04:23 rob Exp $
##  $Name:  $
######################################################################################
    dl('php_mapscript.so');
    require_once 'userlogin.inc';
    require_once 'DB.php';

    $map = ms_newMapObj('tulsa.map');
    #foreach ($_GET as $k => $v) { echo "<!--  " . $k . "=" . $v . "  -->\n"; }

    $gbUConn = new UserLogin();
    if ( !($gbDB = $gbUConn->checkConnection()) ) {
        ## Get the correct extension. We've been using both phtml and php
        header('Location: giserv_login.' .  preg_replace('/^.*\.(\w+)$/', '\1', $_SERVER['PHP_SELF']));
    }

    $error = false;
    if (!$error && isset($_GET['tblname']) ) $tblname = $_GET['tblname'];
    else if (!$error) {
        $error = true;
        $errorMsg = 'Internal data error, invalid tblname';
    }
    if (!$error && isset($_GET['uid']) ) $uid = $_GET['uid'];
    else if (!$error) {
        $error = true;
        $errorMsg = 'Internal data error, invalid uid';
    }

    if (!$error) {
        ## Build select statement without geometry and oid fields
        $sql = "";
        $info = $gbDB->tableInfo($tblname);
        foreach ($info as $k=>$u) {           ## for each field
            if ($info[$k]['type'] != 'oid' && $info[$k]['type'] != 'geometry') {
                if ($sql != '')
                    $sql .= ',';
                $sql .= $info[$k]['name'];
            }
        }
        $sql = "select " . $sql . " from " . $tblname . " where gid=" . $uid;
        $q = $gbDB->query($sql);
        if (DB::isError($q)) echo $gbDB->getMessage() . "\n";
        $row = $q->fetchRow(DB_FETCHMODE_ASSOC);
    }

    ##$info = $gbDB->tableInfo($tblname);
    ##foreach ($info as $k=>$u) {           ## for each field
        ##echo "<br />" . $k . "  " . $info[$k] . "\n";
        ##foreach ($info[$k] as $j=>$v) {   ## for each field attribute
            ##echo "<br /> ---" . $j . " " . $info[$k][$j] . "\n";
        ##}
    ##}
    ##echo "<br />";

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
    <title>2D Seismic Line Details</title>
    <meta HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">

    <style type="text/css" media="all">@import "position.css";</style>
    <script type="text/javascript">
        var NS4DOM = document.layers ? true:false;           // netscape 4
        var IEDOM = document.all ? true:false;               // ie4+
        var W3CDOM = document.getElementById && !(IEDOM) ? true:false;   // netscape 6

    </script>

</head>

<body class="mainstyle">
    <div id="pg_det">
       <div class="pg_det_cls">Close window<a href="javascript:window.close()">&nbsp;X&nbsp;</a></span> </div>
        <span class="pg_det_title">Details</span>
    </div>
    <div id="pg_det_datapnl">
    <?php
        if ($error) echo "<br />Error occurred\n";
        else if ($q->numRows() == 0) echo "<br />No data returned\n";
        else {
            echo "<table border=\"0\" cellpadding=\"2\">\n";
            foreach ($row as $k => $v) {
                echo "<tr><td class=\"det_fldname\">" . ucwords(preg_replace('/_/', ' ', $k))
                   . "</td><td class=\"det_data\">" . $v . "</td></tr>\n";
            }
            $q->free();
            echo "</table>\n";
        }
        ## Embed thumbnail image onto page
        $sql = "select file_name,thumb_name,directory_env,thumb_width,thumb_height"
             . " from vw_seismic_2d_img where gid=" . $uid;

        $q = $gbDB->query($sql);
        if (DB::isError($q)) echo $gbDB->getMessage() . "\n";
        while ( $row = $q->fetchRow(DB_FETCHMODE_ASSOC) ) {
            if (  !isset($row['directory_env']) ) die ("Not set: " . $row['directory_env']);
            ##  Want the destination file to be unique in the tmpdata directory, but also want
            ##  it to be reused by another session if it's needed. So we build it this way:
            ##  <env name>_<file name>  If file name includes a directory name, replace / with _
            $dstname = $row['directory_env'] . '_' . preg_replace( '/\//', '_', $row['thumb_name']);
                                         ## ex: DATA_NERSLWEB_11-74_1174.sgy
            ## copy file from the repository if not in tmp directory
            if ( ! file_exists($map->web->imagepath . $dstname)) {
                $srcname = $_SERVER[$row['directory_env']] . '/' . $row['thumb_name'];
                                         ## ex: /data1/seisdata/nerslweb/11-74/1174.sgy
echo $srcname . "<br />\n";
                if ( ! copy($srcname, $map->web->imagepath . $dstname) ) die ("file copy failed");
            }
                
            echo "<tr><td colspan=\"2\"><img src=\"" . $map->web->imageurl . $dstname . "\" alt=\"thumbnail\""
               . " width=\"" . $row['thumb_width'] . "\"" . $row['thumb_height'] . "\" border=\"1\" /></td></tr>\n";

        }

        $gbDB->disconnect();
    ?>
    </div>
</body>
</html>
