<?php
### ==========================================================================
###   $Id: widget.inc,v 1.1.1.1 2007/04/24 02:04:21 rob Exp $
###   $Name:  $
### ==========================================================================

    require_once 'defs.inc';

### ==========================================================================

    class Widget {
        var $name       = null;
        var $label      = null;
        var $cssClass   = null;
        var $nbrBefore  = 0;
        var $nbrAfter   = 0;

##                   Widget
##                     |
##              -----------------
##              |               |
##            Panel        InputWidget
##                              |
##                -------------------------------------------------------
##                |                          |               |          |
##        RadioButtonGroup               TextField         Button   InputGrid
##                |                          |               |
##        JSRadioButtonGroup            TextListBox      CheckBox
##
##
##
##

        function Widget($name, $label, $cssClass) {
            $this->name = $name;
            $this->label = $label;
            $this->cssClass = $cssClass;
        }

        function setName        ($n)     { $this->name = $n; }
        function getName        ()       { return $this->name; }
        function setLabel       ($l)     { $this->label = $l; }
        function getLabel       ()       { return $this->label; }
        function setCssClass    ($class) { $this->cssClass = $class; }
        function getCssClass    ()       { return $this->cssClass; }
        function setNBr         ($nbefore, $nafter) { $this->nbrBefore = $nbefore; $this->nbrAfter = $nafter; }


        ## These functions should be overridden by derived classes
        function setValuesFromRequest(&$rq_array) { }

        function setKeyWidgetPairs(&$kw) {
            $kw[$this->name] = &$this;
        }

        function validate() { return true; }

        #function htmlWriteBr() {
        #    for ($i=0; $i<$this->nbrBefore; $i++) echo "<br />";
        #    for ($i=0; $i<$this->nbrAfter; $i++) echo "<br />";
        #}
    }

### ==========================================================================

    class Panel extends Widget {
        ## Panel class will contain other objects
        var   $widgetList = array();

        function Panel (  $name, $label, $class  ) {
            $this->name       = $name;
            $this->label      = $label;
            $this->class      = $class;   ## class for label
        }

        function add(&$w) {
            $this->widgetList[  count($this->widgetList)  ] = $w;
        }


        function & getWidgetList() { return $this->widgetList; }

        function htmlWrite() {
            echo "<div id=\"" . $this->name . "\">\n";
            if ($this->label)
                echo "<h3"
                   . ($this->class ? " class=\"" . $this->class . "\"" : "")
                   . ">"
                   . ($this->label ? $this->label : "")
                   . "</h3>\n";

            for ($i=0; $i<count($this->widgetList); $i++) {
                $this->widgetList[$i]->htmlWrite();
            }

            echo "</div>\n";
        }

        function setValuesFromRequest(&$rq_array) {
            ## iterate through the widget tree and set widget's value using the request array
            #foreach ($this->widgetList as $i => $u) {
            for ($i=0; $i<count($this->widgetList); $i++) {
                $this->widgetList[$i]->setValuesFromRequest($rq_array);
            }
        }

        function setKeyWidgetPairs(&$kw) {
            ## Iterate through the widget tree and associate
            ## the widget's name with its object reference.  This allows us to
            ## find the widget by referencing kw with the widget's id
            for ($i=0; $i<count($this->widgetList); $i++) {
                $this->widgetList[$i]->setKeyWidgetPairs($kw);
            }
        }


        function validate() {
            ## Recursively validate all widgets contained by this
            for ($i=0; $i<count($this->widgetList); $i++)
                    $this->widgetList[$i]->validate();
        }
    }

### ==========================================================================

    class PWidget extends Panel {
        ##    <p> tag with objects within </p>


        function PWidget($name, $label, $class) {
            $this->Panel (  $name, $label, $class  );
        }
        

        function htmlWrite() {
            ## Don't output the name. If we need it later we will use id="NAME"
            echo "<p"
               . ($this->class ? " class=\"" . $this->class . "\"" : "")
               . ">"
               . ($this->label ? $this->label : "")
               . "\n";

            for ($i=0; $i<count($this->widgetList); $i++) {
                $this->widgetList[$i]->htmlWrite();
                echo "\n";
            }

            echo "</p>\n";
        }
    }

### ==========================================================================

    class InputWidget  extends Widget {

        var   $value = null;
        var   $errorMsg = null;
        var   $valid = true;
        var   $JSEvent = null;
        var   $JSMethod = null;
        var   $title = null;
        var   $labelPos = DF_LABEL_LEFT;       ## { DF_LABEL_LEFT, DF_LABEL_TOP, DF_LABEL_NONE  }
        var   $blockType = DF_BLOCK_P;   ## { DF_BLOCK_P, DF_BLOCK_TD, DF_BLOCK_NONE }
        var   $blockClass = null;

        function InputWidget ($name, $label, $cssClass, $title, $labelpos, $blockType) {
            $this->Widget($name, $label, $cssClass);
            $this->title = $title;
            $this->labelPos = $labelpos;
            $this->blockType = $blockType;
        }

        function setJSEvent($jse) { $this->JSEvent = $jse; }
        function setJSMethod($jsm) { $this->JSMethod = $jsm; }
        function setTitle($title)  { $this->title = $title; }

        function getJSEvent() { return $this->JSEvent; }
        function getJSMethod() { return $this->JSMethod; }
        function getTitle()    { return $this->title; }

        function setValue($v)  { $this->value = $v; }
        function getValue()    { return $this->value; }

        function setLabelPos($p) { $this->labelPos = $p; }
        function getLabelPos()   { return $this->labelPos; }

        function setBlockType($bt) { $this->blockType = $bt; }
        function getBlockType()  { return $this->blockType; }

        function setBlockClass($c) { $this->blockClass = $c; }
        function getBlockClass()   { return $this->blockClass; }

        function isValid()     { return $this->valid; }

        function getErrorMsg    ()       { return $this->errorMsg; }
    }

### ==========================================================================

    class RadioButtonGroup  extends InputWidget {

        var   $choiceList = array();   ## ( 'val1' => array( 'label1', 'title1'), val2 => array('label2','title2') )
        var   $layout = DF_LAYOUT_VERT;         ## { DF_LAYOUT_HORIZ, DF_LAYOUT_VERT }

        ## Example:
        ## <input type="radio" name="toggle" value="new" onclick="setVw('new');" id="new" />Save new view
        ## <input type="radio" name="toggle" value="del" onclick="setVw('del');" id="del" />Delete view

        function RadioButtonGroup ($name, $label, $cssClass, $title, $labelPos, $blockType, $layout) {
            $this->InputWidget($name, $label, $cssClass, $title, $labelPos, $blockType);
            $this->value = "";
            $this->layout = $layout;
        }

        function add($choice, $label, $title) { 
            if (count($this->choiceList) == 0) $this->value = $choice;
            $this->choiceList[$choice] = array($label, $title);
        }

        function setValue($val) {

            $valid = false;
            foreach ($this->choiceList as $i => $u)
                if ($val == $i) $valid = true;


            if (! $valid) {
                $this->errorMsg = "Invalid value for radio button";
                return;
            }
            $this->value = $val;
        }

        function setValuesFromRequest(&$rq_array) {
            # Set values in button list using request array
            if (array_key_exists($this->name, $rq_array))
                $this->setValue($rq_array[$this->name]);
        }


        ### Use parent class's method instead
        ###function setValuesFromRequest(&$rq_array) {
        ###}

        function htmlWrite() {

            ## The choice list for radio button group is key value pair, where key is the POST value
            ## and value is an array: (label, title)
            
            ##  Allow for vertical and horizontal layout (DF_LAYOUT_VERT, DF_LAYOUT_HORIZ)

                 #### DF_BLOCK_TD, so far, is meaningless and will be ignored
            if ($this->blockType == DF_BLOCK_P)
                echo "<p" . ($this->cssClass ? " class=\"". $this->cssClass . "\"" : "") . ">";

            if ($this->labelPos != DF_LABEL_NONE) echo $this->label;

            if ($this->labelPos == DF_LABEL_TOP) echo "<br />";

            $firstTime = true;
            foreach ($this->choiceList as $i => $u) {
                if (! $firstTime && $this->layout == DF_LAYOUT_VERT)
                    echo "<br />";

                echo "<input type=\"radio\" name=\"" . $this->name . "\""
                   . " id=\"" . $this->name . "_" . strtolower($i) . "\""
                   . " value=\"" . $i  . "\""
                   . ($this->choiceList[$i][1] ? " title=\"" . $this->choiceList[$i][1] . "\"" : "");


                ## Note, Special case for dealing with js event. Use $value for the arg of the method
                #echo ($this->JSEvent ? " " . $this->JSEvent . "=\"" . $this->JSMethod . "\"" : "");
                #echo ($this->JSEvent ? " " . $this->JSEvent . "=\"" . $this->JSMethod . "('" . $i . "');\"" : "");
                echo ($this->JSEvent ? " " . $this->JSEvent . "=\"" . $this->JSMethod ."\"" : "");


                echo ($this->value == $i ? " checked=\"checked\"" : "") . " />";

                echo ($this->choiceList[$i][0] ? $this->choiceList[$i][0] : "") . "\n";

                if ($firstTime) $firstTime = false;
            }
            if ($this->blockType == DF_BLOCK_P) echo "</p>\n";
        }
    }

### ==========================================================================

    class JSRadioButtonGroup extends RadioButtonGroup {

        var $upClass = null;
        var $dnClass = null;

        function JSRadioButtonGroup ($name, $label, $cssClass, $title, $labelPos, $blockType, $layout,
                                     $upClass, $dnClass) {

            $this->RadioButtonGroup ($name, $label, $cssClass, $title, $labelPos, $blockType, $layout);
            $this->value = "";
            $this->upClass = $upClass;
            $this->dnClass = $dnClass;
        }

        function add($id, $label, $title, $image) { 

            ##                       0         1        2
            ## ( 'val1' => array( 'label1', 'title1','image1'), val2 => array('label2','title2','image2') )
            if (count($this->choiceList) == 0) $this->value = $id;  ## Set value to first
            $this->choiceList[$id] = array($label, $title, $image);
        }


        function htmlWrite() {
            
            echo "<table"
               . " id=\"tbl_" . $this->name . "\""
               . ($this->cssClass ? "class=\"" . $this->cssClass . "\"" : "")
               . ">\n";
            echo "<tr>\n";

            ## Do for each button in list
            foreach ($this->choiceList as $i => $u) {
                echo "<td align=\"center\">";
                echo "<a href=\"#\""
                   . ($this->choiceList[$i][1] ? " title=\"" . $this->choiceList[$i][1] . "\"" : "")
                   . ">";
                echo "<img id=\"" . $i .  "\""
                   . " class=\"" . ($i == $this->value ? $this->dnClass : $this->upClass) . "\""
                   . " src=\"" . $this->choiceList[$i][2] .  "\""
                   . " alt=\"" . $this->choiceList[$i][1] . "\""
                   . " width=\"16\" height=\"16\""
                   . " onclick=\"btnOnclick('" . $i ."');return false\" />";
                echo "</a>";

                echo "</td>";
                echo "\n";
            }

            echo "</tr>\n";
            echo "</table>\n";

            ## Use a hidden field here since these image buttons and don't send anything to server
            ## Must use javascript to set values.  This hidden field does the sending
            echo "<input type=\"hidden\" name=\"" . $this->name   . "\""
               . " value=\"" .  $this->value   . "\""
               . " id=\"" . $this->name . "\" />\n";
        }
    }

### ==========================================================================
    class TextField extends InputWidget {

        var $dataType   = DF_CHR;
        var $valid      = true;
        var $size;                 ## Size of text box
        var $maxlength;
        var $isRequired = false;   ## Indicates that field is required
        var $isReadonly = false;
        var $isHidden = false;
        var $stringCase = null;    ## If non-null, string values are changed
                                   ## to this case for internal storage
                                   ## after value setting functions
                                   ## like the constructor TextField or
                                   ## setValue are called

##function addTextField($name, $label, $cssClass, $datatype, $size, $value)

    ## ------------------------------------------------------------------------
        function TextField( $name, $label, $cssClass, $title, $labelPos, $blockType, $dataType, $size, $value ) {

            ## Call the constructor of the parent class
            $this->InputWidget($name, $label, $cssClass, $title, $labelPos, $blockType);

            $this->size = $size;    
            $this->toUniformCase();
             
            $this->dataType = $dataType;
            $this->maxlength = $this->size;

            $len = strlen($value);
            $len = $len < $this->maxlength ? $len : $this->maxlength;

            $this->value = substr($value, 0, $len);
        }

    ## ------------------------------------------------------------------------

        function setValuesFromRequest(&$rq_array) {
            # Set value using request array
            if (array_key_exists($this->name, $rq_array) && $rq_array[$this->name]) {
                $this->value = $rq_array[$this->name];
                $len = strlen($rq_array[$this->name]);
                $len = $len < $this->maxlength ? $len : $this->maxlength;
                $this->value = substr($rq_array[$this->name], 0, $len);
                $this->validate();
            }
        }


        ### Use parent class's method instead
        ###function setValuesFromRequest(&$rq_array) {
        ###}

    ## ------------------------------------------------------------------------

        function setSize        ($s)     { $this->size = $s;    }
        function getSize        ()       { return $this->size; }

        function setMaxlength   ($m)     { $this->maxlength = $m; }
        function getMaxlength   ()       { return $this->maxlength; }

        function setRequired    ($r)     { $this->isRequired = $r; }
        function getRequired    ()       { return $this->isRequired; }  

        function setCase        ($c)     { $this->stringCase = $c; }
        function getCase        ()       { return $this->stringCase; }

        function setReadonly    ($r)     { $this->isReadonly = $r; }
        function setHidden      ($v)     { $this->isHidden = $v; }

    ## ------------------------------------------------------------------------
        function toUniformCase() {
            if ($this->dataType != DF_CHR || $this->value == "" || !isset($this->stringCase) ) return;

            if ($this->stringCase == DF_UCASE) $this->value = strtoupper($this->value);
            elseif ($this->stringCase == DF_LCASE) $this->value = strtolower($this->value);
        }
    ## ------------------------------------------------------------------------

        function validate() {
            $this->toUniformCase();
            ##echo $this->name . "  {" . $this->label . "} value:{" . $this->value . "}  req:{" . $this->isRequired . "}<br />\n";
    
            $this->valid = true;
            if ($this->isReadonly) return $this->valid;

            #echo "<!--  valid=" . $this->valid . "    -->\n";
            if ($this->isRequired &&  ($this->value == "" || is_null($this->value))) {
                $this->valid = false;
                $this->errorMsg = "Required";
                return $this->valid;
            }
            elseif ($this->value == "" || is_null($this->value))
                return $this->value;
            switch ($this->dataType) {

                case DF_INT:
                if ( ! ereg("^[+\-]?[0-9]+$", $this->value) ) {
                    $this->valid = false;
                    $this->errorMsg = "Invalid integer value";
                }
                else {
                    $this->valid = true;
                    #$this->value = intval($this->value);
                }
                break;

                case DF_DBL:
                if ( ! is_numeric($this->value) ) {
                    $this->valid = false;
                    $this->errorMsg = "Invalid decimal value";
                }
                else {
                    $this->valid = true;
                    $this->value = doubleval($this->value);
                }
                break;

                case DF_CHR:
                $this->valid = true;
                break;
            }
            return $this->valid;
        }

        function htmlWrite() {

            for ($i=0; $i<$this->nbrBefore; $i++) echo "<br />";

            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">";
            }
            if ($this->labelPos != DF_LABEL_NONE) echo  $this->label;
            if ($this->labelPos == DF_LABEL_TOP) echo "<br />";

            echo "<input type=\"" . ($this->isHidden ? "hidden" : "text") . "\""
               . " id=\"" . $this->name . "\" name=\"" . $this->name . "\""
               . ($this->cssClass ? " class=\"" . $this->cssClass . "\"" : "")
               . ($this->title ? " title=\"" . $this->title . "\"" : "")
               . " value=\"" . $this->value . "\""
               . ($this->isHidden ? "" : " size=\"" . $this->size . "\" maxlength=\"" . $this->maxlength . "\"")
               . ($this->JSEvent ? " " . $this->JSEvent . "=\"" . $this->JSMethod . "\"" : "")
               . ($this->isReadonly ? " readonly=\"readonly\"" : "")
               . " />";

            if ($this->blockType != DF_BLOCK_NONE) {
                if ( ! $this->valid )
                    echo "<span class=\"err_invalid\">" . $this->errorMsg . "</span>";

                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");
            }
            for ($i=0; $i<$this->nbrAfter; $i++) echo "<br />";
            echo "\n";
        }
    }


    ### ==========================================================================

    class TextListBox extends TextField {
        var $validList = array();
        var $nToDisp = null;
        var $noValidate = false;

        ### Note, the "size" attribute indicates the number of rows displayed in
        ### the list box and not the length of the field

        function TextListBox( $name, $label, $cssClass, $title, $labelPos, $blockType, $dataType, $size, $value) {
            $this->nToDisp = $size;
            $size = 20;  ## Not used by <select> tag. Reset to something so string is not truncated
                         ## by setValue() functions in parent class
            $this->TextField( $name, $label, $cssClass, $title, $labelPos, $blockType, $dataType, $size, $value );
        }

        function add( $key, $value ) { $this->validList[$key] = $value; }

        function setNoValidate() { $this->noValidate = true; }

        function validate() {
            if ($this->noValidate) { $this->valid = true;  return $this->valid; }
            if (!parent::validate()) return false;

            if (array_key_exists($this->value, $this->validList)) {
                $this->valid = true;
                return $this->valid;
            }
            $this->valid = false;
            $this->errorMsg = "Invalid value";
            return $this->valid;
        }


        function htmlWrite() {
            for ($i=0; $i<$this->nbrBefore; $i++) echo "<br />";

            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">";
            }

            if ($this->labelPos != DF_LABEL_NONE) echo  $this->label;
            if ($this->labelPos == DF_LABEL_TOP) echo "<br />";

            echo "<select id=\"" . $this->name . "\" name=\"" . $this->name . "\""
               . ($this->cssClass ? " class=\"" . $this->cssClass . "\"" : "")
               . ($this->title ? " title=\"" . $this->title . "\"" : "")
               . " size=\"" . $this->nToDisp . "\""
               . ($this->JSEvent ? " " . $this->JSEvent . "=\"" . $this->JSMethod . "\"" : "")
               . ">\n";

            foreach ($this->validList as $i => $u) {
                $sel =  $this->value == $i ? " selected=\"selected\"" : "";
                echo "    <option value=\"" . $i . "\"" . $sel . ">" . $u . "</option>\n";
            }

            echo "</select>\n";
            if ($this->blockType != DF_BLOCK_NONE) {
                if ( ! $this->valid )
                    echo "<span class=\"err_invalid\">" . $this->errorMsg . "</span>";

                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");
            }
            for ($i=0; $i<$this->nbrAfter; $i++) echo "<br />";
            echo "\n";

        }

        function setValidList(&$list) {  $this->validList = &$list; }
    }
    ### ==========================================================================

    class Button extends InputWidget {

        function Button ($name, $label, $cssClass, $title, $labelPos, $blockType) {
            $this->InputWidget($name, $label, $cssClass, $title, $labelPos, $blockType);
            
            ## Label position will be ignored
        }


        function setValuesFromRequest(&$rq_array) {
            # Set values in button list using request array
            if (array_key_exists($this->name, $rq_array))
                $this->setValue("Y");
        }


        function htmlWrite() {
            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">";
            }

            ##if ($this->labelPos != DF_LABEL_NONE) echo  $this->label;
            ##if ($this->labelPos == DF_LABEL_TOP) echo "<br />";

            echo "<input type=\"submit\""
               . ($this->cssClass ? " class=\"" . $this->cssClass . "\"" : "")
               . " id=\"" . $this->name . "\""
               . " name=\"" . $this->name . "\""
               . ($this->title ? " title=\"" . $this->title . "\"" : "")
               . " value=\"" . $this->label . "\""
               . " />";


            if ($this->blockType != DF_BLOCK_NONE)
                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");

            echo "\n";
        }
    }
### ==========================================================================


    class InputGrid  extends InputWidget {

        var   $layout = DF_LAYOUT_ARRAY;  ## { DF_LAYOUT_ARRAY,  DF_LAYOUT_CELL }
        var   $nrows;
        var   $ncols = 0;    ## For layout type=CELL, this is incremented by this->add()
        var   $cells = array();
        var   $colHdrs = array();
        var   $XXX = null;       ## Reserve for header text class

              ## Notes:   column headings taken from label string of widget. However, the
              ##          labelPos argument is ignored.


        function InputGrid ($name, $label, $cssClass, $title, $labelPos, $blockType, $layout, $nrows) {
            $this->InputWidget($name, $label, $cssClass, $title, $labelPos, $blockType);
            $this->layout = $layout;
            $this->nrows = $nrows;
        }

        function add($w) {   ## we pass by reference, but we copy the obj to the array
           ## note, as of php5, & not necessary if $w was created using new in the caller

            ## Define
            $this->colHdrs[$this->ncols] = $w->getLabel();

            ## Create a new array for this column and replicate the widget "nrows" times
            $this->cells[ $this->ncols ] = array();
            for ($j=0; $j<$this->nrows; $j++) {

                ## Need to do a deep copy (if there is such a thing in php)
                ## Since $w was created using new, need to use clone.

                ##$this->cells [ $this->ncols ][$j] = $w;
                $this->cells [ $this->ncols ][$j] = clone $w;      ## dangerously assume only a shallow copy required
                $this->cells [ $this->ncols ][$j]->setLabel(null);
                ## $name = $w->getName() . sprintf("%d", $j);
                $name = $this->cells[$this->ncols][$j]->getName() . sprintf("%d", $j);

                $this->cells [ $this->ncols ][$j]->setName($name);
            }
            $this->ncols++;
        }


        function setValuesFromRequest(&$rq_array) {
            for ($i=0; $i<$this->ncols; $i++) {
                for ($j=0; $j<$this->nrows; $j++) {
                    $this->cells[$i][$j]->setValuesFromRequest($rq_array);
                }
            }
        }

        function setKeyWidgetPairs(&$kw) {
            for ($i=0; $i<$this->ncols; $i++) {
                for ($j=0; $j<$this->nrows; $j++) {
                    $this->cells[$i][$j]->setKeyWidgetPairs($kw);
                }
            }
        }


        function validate() {
            for ($i=0; $i<$this->ncols; $i++) {
                for ($j=0; $j<$this->nrows; $j++) {
                    $this->cells[$i][$j]->validate();
                }
            }
        }


        function htmlWrite() {


            ## -----------   Generate headings for table   ---------- ##
            echo "<table" . ($this->cssClass ? " class=\"" . $this->cssClass . "\"" : "") . ">\n";
            echo "<tr"
               . ">";
            for ($i=0; $i<$this->ncols; $i++) {
                echo "<td" . ($this->XXX ? " class=\"" . $this->XXX . "\"" : "") . ">"
                   . $this->colHdrs[$i] . "</td>";
            }
            echo "</tr>\n";


            ## -----------        Write table rows         ---------- ##
            ##
            ## Note, programmer needs to ensure that direct decendent
            ##  child widgets have block type of DF_BLOCK_TD
            ##  That's because this function does not generate <td> tags
            ##
            for ($j=0; $j<$this->nrows; $j++) {
                echo "<tr"
                   . ">\n";
                for ($i=0; $i<$this->ncols; $i++) {
                    $this->cells[$i][$j]->htmlWrite();
                }
                echo "</tr>\n";

            }
            echo "</table>\n";
        }

    }

    ### ==========================================================================

    class CheckBox extends Button {

        function CheckBox ($name, $label, $cssClass, $title, $labelPos, $blockType) {
            $this->Button($name, $label, $cssClass, $title, $labelPos, $blockType);
            
            ## Label position will be ignored
        }


        function setValuesFromRequest(&$rq_array) {
            # Set values in button list using request array
            if (array_key_exists($this->name, $rq_array))
                if ($rq_array[$this->name] == "Y")
                    $this->setValue("Y");
                else
                    $this->setValue("");
        }


        function htmlWrite() {
            for ($i=0; $i<$this->nbrBefore; $i++) echo "<br />";
            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">";
            }

            if ($this->labelPos != DF_LABEL_NONE) echo  $this->label;
            if ($this->labelPos == DF_LABEL_TOP) echo "<br />";

            echo "<input type=\"checkbox\""
               . ($this->cssClass ? " class=\"" . $this->cssClass . "\"" : "")
               . " id=\"" . $this->name . "\""
               . " name=\"" . $this->name . "\""
               . ($this->title ? " title=\"" . $this->title . "\"" : "")
               . " value=\"Y\""
               . ($this->value == "Y" ? " checked=\"checked\"" : "")
               . " />";


            if ($this->blockType != DF_BLOCK_NONE)
                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");

            for ($i=0; $i<$this->nbrAfter; $i++) echo "<br />";
            echo "\n";
        }
    }
    ### ==========================================================================

    class TextWidget extends Widget {

        var $blockClass = null;

        function TextWidget($name, $label, $class, $blockType) {

            $this->Widget($name, $label, null);   ## set class to null in parent
            $this->class = $class;
            $this->blockType = $blockType;
        }


        function setValue($v)  { }
        function getValue()    { return null; }

        function setBlockType($bt) { $this->blockType = $bt; }
        function getBlockType()  { return $this->blockType; }

        function setBlockClass($c) { $this->blockClass = $c; }
        function getBlockClass()   { return $this->blockClass; }


        function htmlWrite() {
            
            for ($i=0; $i<$this->nbrBefore; $i++) echo "<br />";
            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">";
            }
            echo $this->label;

            if ($this->blockType != DF_BLOCK_NONE)
                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");
            for ($i=0; $i<$this->nbrAfter; $i++) echo "<br />";

            #echo "\n";

        }
    }
    ### ==========================================================================

    class ImageWidget extends TextWidget {

        var $src = null;
        var $width = 16;
        var $height = 16;
        var $title = null;
        var $imageMap = null;

        ## Use parent constructor
        function ImageWidget($name, $label, $class, $blockType, $src, $width, $height, $title) {
            $this->TextWidget($name, $label, $class, $blockType);
            $this->src = $src;
            $this->width = $width;
            $this->height = $height;
            $this->title = $title;
        }

        function setSrc($src, $width, $height, $title) {
            $this->src = $src;
            $this->width = $width;
            $this->height = $height;
            $this->title = $title;
        }

        function getSrc() { return $this->src; }
        function getWidth() { return $this->width; }
        function getHeight() { return $this->title; }

        function setImageMap(&$imageMap) { $this->imageMap = &$imageMap; }


        function htmlWrite() {
            
            for ($i=0; $i<$this->nbrBefore; $i++) echo "<br />";
            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">\n";
            }

            echo "<img src=\"" . $this->src . "\""
               . " id=\"" . $this->name . "\""
               . " width=\"" . $this->width . "\""
               . " height=\"" . $this->height . "\""
               . " alt=\"" . $this->title . "\""
               . ($this->imageMap ? " usemap=\"#" . $this->imageMap->getName() . "\"" : "")
               . " />\n";

            if ($this->imageMap)
                $this->imageMap->htmlWrite();


            if ($this->blockType != DF_BLOCK_NONE)
                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");

            for ($i=0; $i<$this->nbrAfter; $i++) echo "<br />";
            echo "\n";

        }
    }
### ==========================================================================


    class RowWidget  extends InputWidget {

        var   $widgList = array();
        var   $nspnList = array();
        var   $classList = array();


        function RowWidget ($name, $label, $cssClass, $title, $labelPos, $blockType) {
            $this->InputWidget($name, $label, $cssClass, $title, $labelPos, $blockType);
        }

        function add(&$w, $class=null, $nspan=0) {   ## we pass by reference, but we copy the obj to the array
                                        ## if nspan = 0, defaults to 1
            ## add a cell ( <td> cell </td> ) to a row

            $this->widgList[ count($this->widgList) ] =  $w;
            $this->nspnList[ count($this->nspnList) ] = $nspan;
            $this->classList[ count($this->classList) ] = $class;
        }

        function htmlWrite() {
            
            ## Ignore blockType
            ## blockClass is for 


            for ($i=0; $i<count($this->widgList); $i++) {
                echo "<td"
                   . ($this->classList[$i] ? " class=\"" . $this->classList[$i] . "\"" : "")
                   . ($this->nspnList[$i] ? " colspan=\"" . $this->nspnList[$i] . "\"" : "")
                   . ">";
                $this->widgList[$i]->htmlWrite();
                echo "</td>\n";
            }


        }


        ## These functions should be overridden by derived classes
        function setValuesFromRequest(&$rq_array) {

            for ($i=0; $i<count($this->widgList); $i++) {
                $this->widgList[$i]->setValuesFromRequest($rq_array);
            }
        }


        function setKeyWidgetPairs(&$kw) {

            for ($i=0; $i<count($this->widgList); $i++) {
                $this->widgList[$i]->setKeyWidgetPairs($kw);
            }
        }

        function validate() {
            for ($i=0; $i<count($this->widgList); $i++) {
                $this->widgList[$i]->validate();
            }
        }
    }

    ### ==========================================================================


    class TableWidget  extends InputWidget {

        var   $rowList = array();
        var   $classList = array();
        var   $cellspacing = -1;


        #function TableWidget ($name, $label, $cssClass, $title, $labelPos, $blockType) {
        #    $this->InputWidget($name, $label, $cssClass, $title, $labelPos, $blockType);
        #}

        function add(&$w, $class=null) {   ## we pass by reference, but we copy the obj to the array
            $this->rowList[ count($this->rowList) ] =  $w;
            $this->classList[ count($this->classList) ] =  $class;
        }

        function setCellspacing($n) { $this->cellspacing = $n; }

        function htmlWrite() {
            
            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">";
            }

            echo "<table id=\"" . $this->name . "\""
               . ($this->cellspacing > -1 ? " cellspacing=\"" . $this->cellspacing . "\"" : "")
               . ($this->cssClass ? " class=\"" . $this->cssClass . "\"" : "")
               . ">\n";

            for ($i=0; $i<count($this->rowList); $i++) {
                echo "<tr"
                ##   . " id=\"" . $this->name . "_" . $i . "\""
                   . " id=\"" . $this->rowList[$i]->name . "\""
                   . ($this->classList[$i] ? " class=\"" . $this->classList[$i] . "\"" : "")
                   . ">\n";
                $this->rowList[$i]->htmlWrite();
                echo "</tr>\n";
            }
            echo "</table>\n";

            if ($this->blockType != DF_BLOCK_NONE)
                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");

            echo "\n";
        }


        ## These functions should be overridden by derived classes
        function setValuesFromRequest(&$rq_array) {

            for ($i=0; $i<count($this->rowList); $i++) {
                $this->rowList[$i]->setValuesFromRequest($rq_array);
            }
        }


        function setKeyWidgetPairs(&$kw) {

            for ($i=0; $i<count($this->rowList); $i++) {
                $this->rowList[$i]->setKeyWidgetPairs($kw);
            }
        }

        function validate() {
            for ($i=0; $i<count($this->rowList); $i++) {
                $this->rowList[$i]->validate();
            }
        }
    }
    ### ==========================================================================

    class InputImage extends InputWidget {
        var $src = null;

        function InputImage ($name, $label, $cssClass, $title, $labelPos, $blockType, $src) {
            $this->InputWidget($name, $label, $cssClass, $title, $labelPos, $blockType);
            $this->src = $src;
            $this->value = null;   ## Don't need a value for this (yet)
        }

        function setSrc($src, $title=null) {
            $this->src = $src;
            if ($title) $this->title = $title;
        }


        function setValuesFromRequest(&$rq_array) {
            # Disable, since this is meaningless for this class at this time
            ## But still need a dummy function
            #if (array_key_exists($this->name, $rq_array))
            #    $this->setValue("Y");
        }


        function htmlWrite() {
            for ($i=0; $i<$this->nbrBefore; $i++) echo "<br />";
            if ($this->blockType != DF_BLOCK_NONE)  {    ## Block type is paragraph or table cell
                echo ($this->blockType == DF_BLOCK_P ? "<p" : "<td")
                   . ($this->blockClass ? " class=\"" . $this->blockClass . "\"" : "") . ">";
            }

            ##if ($this->labelPos != DF_LABEL_NONE) echo  $this->label;
            ##if ($this->labelPos == DF_LABEL_TOP) echo "<br />";

            echo "<input type=\"image\""
               . ($this->cssClass ? " class=\"" . $this->cssClass . "\"" : "")
               . " src=\"" . $this->src . "\""
               . " id=\"" . $this->name . "\""
               . " name=\"" . $this->name . "\""
               . ($this->title ? " title=\"" . $this->title . "\"" : "")
               . ($this->title ? " alt=\"" . $this->title . "\"" : "")
               . ($this->JSEvent ? " " . $this->JSEvent . "=\"" . $this->JSMethod . "\"" : "")
               . " />";


            if ($this->blockType != DF_BLOCK_NONE)
                echo ($this->blockType == DF_BLOCK_P ? "</p>" : "</td>");

            for ($i=0; $i<$this->nbrAfter; $i++) echo "<br />";
            echo "\n";
        }
    }

    ### ==========================================================================

    class ImageMap {

        var    $name;
        var    $JSmover = null;   ## Javascript on mouse over method
        var    $JSmout  = null;   ## Javascript on mouse out method
        var    $JSclick = null;   ## Javascript on click method
        var    $areaTags = null;

        function ImageMap($name, $JSmover, $JSmout, $JSclick) {
            $this->name = $name;
            $this->JSmover = $JSmover;
            $this->JSmout = $JSmout;
            $this->JSclick = $JSclick;
        }

        function getName() { return $this->name; }

        function addArea($code, $text, $coords) {
            if (!$this->areaTags) $this->areaTags = array();
            $n = count($this->areaTags);
            $this->areaTags[ $n ] = array( "code"=>$code, "text"=>$text, "xcoords"=>array(), "ycoords"=>array());
            $tmparray = split(' *, *', $coords);
            $i = 0;
            while ($i < count($tmparray)) {
                $this->areaTags[$n]["xcoords"][$i/2] = $tmparray[$i];
                $this->areaTags[$n]["ycoords"][$i/2] = $tmparray[$i+1];
                $i += 2;
            }
        }


        function htmlWrite() {

            echo "<map name=\"" . $this->name . "\" id=\"" . $this->name . "\">\n";


            for ($i=0; $i<count($this->areaTags); $i++) {
                echo "    <area shape=\"poly\" coords=\"";
                $ncoords = count($this->areaTags[$i]["xcoords"]);

                for ($j=0; $j<$ncoords; $j++) {
                    if ($j > 0) echo ",";
                    echo $this->areaTags[$i]["xcoords"][$j] . "," . $this->areaTags[$i]["ycoords"][$j];
                }
                echo "\""
                   . ($this->JSmover ? " onmouseover=\"return " . $this->JSmover . "(this,'" . $this->areaTags[$i]["code"] . "');\"" : "")
                   . ($this->JSmout ? " onmouseout=\"return " . $this->JSmout . "(this,'" . $this->areaTags[$i]["code"] . "');\"" : "")
                   . ($this->JSclick ? " onclick=\"return " . $this->JSclick . "(this,'" . $this->areaTags[$i]["code"] . "');\"" : "")
                   . " alt=\"" . $this->areaTags[$i]["text"] . "\""
                   . " nohref=\"nohref\""
                   . " />\n";
            }

            echo "</map>\n";
        }
    }

    ### ==========================================================================
?>
