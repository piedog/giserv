<?php
##===============================================================================
##  $Id: giserv_login.php,v 1.1.1.1 2007/04/24 02:04:23 rob Exp $
##  $Name:  $
##===============================================================================
    require_once 'userlogin.inc';
    require_once 'defs.inc';

    $user = new UserLogin();

    $initial = false;
    $loginErrMsg = '';
    $loginStatus = false;
    if (!isset($_POST['self_error']) && (!isset($_GET['redir_error']) || $_GET['redir_error'] == 'none') ) {
        ## If get variable not set, then this is initial login and
        ## there is no need for error messages
        ## If redir_error == 'none', then the application redirected to here after
        ## a logout.
        $initial = true;
    }

    else if (isset($_GET['redir_error'])) {
        $loginErrMsg = $_GET['redir_error'];
    }

    else if (!isset($_POST['lg_txt_uname']) || !isset($_POST['lg_txt_pass'])) {
        $loginErrMsg = 'Userid and password required to login';
    }

    else if (isset($_POST['lg_txt_uname']) && $_POST['lg_txt_uname'] != ""
        && isset($_POST['lg_txt_pass']) && $_POST['lg_txt_pass'] != "") {

        ## Now login with the real username, password
        if ($user->validateUser($_POST['lg_txt_uname'], $_POST['lg_txt_pass'])) {
            ## Redirect to the giserv application
                header('Location: giserv.' . preg_replace('/^.*\.(\w+)$/', '\1', $_SERVER['PHP_SELF']));
        }
        else
            $loginErrMsg = 'Access denied';
    }
##===============================================================================
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<meta HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<title>Web map login screen</title>
<meta name="keywords" content="Geophysics, seismic data">
<meta name="description" content="GIS mapserver">
     
<style type="text/css" media="all">@import "login.css";</style>
<script language="JavaScript" type="text/javascript">
<!--
        var NS4DOM = document.layers ? true:false;           // netscape 4
        var IEDOM = document.all ? true:false;               // ie4+
        var W3CDOM = document.getElementById && !(IEDOM) ? true:false;   // netscape 6
        var newWindow;

        function opDetailWindow() {
            var windowFeatures;
            var xwd = 400;
            var yht = 500;
            var scx = 20;
            var scy = 50;
            if (W3CDOM) {
               windowFeatures = "width=" + xwd + ",height=" + yht;
               windowFeatures += ",resizable";
               windowFeatures += ",scrollbars";
               windowFeatures += ",screenX=" + scx + ",screenY=" + scy;
            }
            else {
               windowFeatures = "directories=0,location=0,menubar=0,resizeable=1,scrollbars=1,status=1,toolbar=0";
               windowFeatures += ",width=" + xwd + ",height=" + yht;
            }
            // ------------------------------------------------
            var url = 'tutorial.html';
            var name = 'tutorialWindow';
            if (typeof(newWindow) != "undefined") newWindow.focus();
            newWindow = window.open(url,name,windowFeatures);
        }
        // -->
</script>

</head>
<body class="mainstyle">
<div id="lg_pg">
    <form method=post name="login" action=<?php echo "\"".$_SERVER["PHP_SELF"]."\"";?> >
    <div id="leftpanel">
        <h3><span class="emph">Please Note Browser Limitations</span></h3>
        This site has been tested with the following browsers:
        <ul>
            <li>Mozilla 1.5</li>
            <li>Internet Explorer 5.5</li>
            <li>Internet Explorer 6.0</li>
        </ul>
        <br />
        For this site to properly work, please enable the following settings in your browser:
        <ul>
            <li>JavaScript</li>
            <li>Cookies</li>
            <li>Popup Windows</li>
        </ul>
        <p class="version_str">
            <?php
                echo "giserv version: " . giserv_getVersionString();
                echo "<br />". "\n";
            ?>
        <p class="copyright">
            Copyright &#064;2005 DataFlow Design, Inc. All rights reserved.
        </p>
    </div>
    <!-- =================================================================== -->

    <div id="lg_pnl_login">
        <h3>Login to Giserv</h3>
        <br />
        <?php
            echo "<br />Enter user name: <br /><input type=\"text\" name=\"lg_txt_uname\""
               . " id=\"lg_txt_uname\" value=\""
               . (isset($_POST['lg_txt_uname']) ? $_POST['lg_txt_uname'] : '')
               . "\" size=20 />\n";
            echo "<br />\n";
            echo "<br />Enter password: <br /><input type=\"password\" name=\"lg_txt_pass\""
               . " id=\"lg_txt_pass\" value=\""
               . (isset($_POST['lg_txt_pass']) ? $_POST['lg_txt_pass'] : '')
               . "\" size=20 />\n";
        ?>
        <br />
        <br /><input type="submit" name="sub_login" id="lg_sub_login" value="Login" />
              <input type="hidden" name="self_error" id="self_error" value="1" />
    </div>
    <div id="lg_pnl_pass">
        <br />
        <br /> <span class="emph">If you do not have a login, you may use:</span>
        <br />
        <br /> User Name: <span class="examp">guest</span>
        <br /> Password: <span class="examp">guest</span>
    </div>
    <div id="lg_pnl_first">
        <br />
        <br />If you are a first time user, this
        <a href="#" onclick="opDetailWindow('vw_seismic_2d','21');return false;">tutorial</a>
        may get you started.
    </div>


    <!-- =================================================================== -->
    <div id="lg_pnl_msg">
        <?php
            echo "<p><h2>" . $loginErrMsg . "</h2></p>\n";
            echo "<input type=\"hidden\" id=\"self_error\" name=\"self_error\" value=\"1\" />\n";
        ?>
    </div>
    </form>
</div>
</body>
</html>
