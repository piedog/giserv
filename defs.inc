<?php
### ==========================================================================
###   $Id: defs.inc,v 1.1.1.1 2007/04/24 02:04:22 rob Exp $
###   $Name:  $
### ==========================================================================

    define ('DF_CHR', 0);
    define ('DF_INT', 1);
    define ('DF_DBL', 2);

    define ('DF_UCASE',0);
    define ('DF_LCASE',1);

    define ('DF_LABEL_NONE', 0);
    define ('DF_LABEL_LEFT', 1);
    define ('DF_LABEL_TOP', 2);
    define ('DF_LABEL_OVER',3);

    define ('DF_BLOCK_NONE', 0);
    define ('DF_BLOCK_P', 1);
    define ('DF_BLOCK_TD', 2);    ## Deprecate this
    define ('DF_BLOCK_SPAN', 3);

    define ('DF_LAYOUT_VERT', 0);
    define ('DF_LAYOUT_HORIZ', 1);
      ## Not used or used inconsistently
    define ('DF_LAYOUT_ARRAY', 2);   ## For InputGrid, each row is cloned
    define ('DF_LAYOUT_CELL', 3);    ## For InputGrid, rows are not repeated

    ## Tree defs
    define ('DF_TREE_NULL', 0);
    define ('DF_TREE_SELF', 1);
    define ('DF_TREE_CHILD', 2);
    define ('DF_TREE_LASTCHILD', 3);
    define ('DF_TREE_INDIRECT', 4);

### ==========================================================================
 
    function giserv_GetVersionString() {
        $str = '$Name:  $';
        $regx = '/.*\$'    ## Avoid CVS keyword exapansion by defining the regexp in a string variable
              . 'Name:? *([0-9_\-A-Za-z]*) *'
              . '\$.*/'; 
        $vers = preg_replace($regx, '$1', $str);
        if (! $vers || $vers == "") $vers = "WORK";
        return $vers;
    }

### ==========================================================================
?>
