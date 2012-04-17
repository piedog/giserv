<?php
    ######################################################################
    ##   $Id: tree.inc,v 1.1.1.1 2007/04/24 02:04:20 rob Exp $
    ##   $Name:  $
    ######################################################################

    # =====================================================================

    class Tree {
        var $name = null;
        var $label = null;
        var $type  = null;
        var $layIdx = -1;
        var $clsIdx = -1;
        var $htmlName = null;
        var $imgName = null;    ## associate this node with the html elem name of the image for expand/collapse

        var $children = null;   ## If null, then this is a leaf node

        function Tree($name, $label, $type, $layerIdx=-1, $classIdx=-1) {
            $this->name = $name;
            $this->label = $label;
            $this->type = $type;
            $this->layIdx = $layerIdx;
            $this->clsIdx = $classIdx;
        }

        function add( &$newNode ) {
            if ( ! $this->children ) $this->children = array();

            $this->children[ count($this->children) ] = &$newNode;
#echo "Tree: adding ".$this->children[count($this->children)-1]->name." to ".$this->name."<br />\n";
        }

        function write() {
            echo $this->name . ": "
               . ", " . $this->label
               . ", layIdx=" . $this->layIdx
               . ", clsIdx=" . $this->clsIdx
               . ", " . ($this->children ? "num children=" . count($this->children) : "leaf")
               . "<br />\n";
            if (! $this->children) return;
            echo "{";
            for ($i=0; $i<count($this->children); $i++)
                $this->children[$i]->write();
            echo "}";
        }

        function writeJS_debug() {
            echo $this->htmlName . ": "
               . "<br />\n";
            if (! $this->children) return;
            echo "{";
            for ($i=0; $i<count($this->children); $i++)
                $this->children[$i]->writeJS_debug();
            echo "}";
        }

        function writeJSString($isRoot=1) {

            ## The root node must be global, so don't put a "var" before the variable name
            echo ($isRoot?"":"var ") . $this->htmlName . "= new LayerNode(\"" . $this->htmlName . "\");\n";
            echo $this->htmlName . ".setImageName(\"" . $this->imgName . "\");\n";

            for ($i=0; $i<count($this->children); $i++) {

                $node = &$this->children[$i];
                if (!$node->children) {
                    echo $this->htmlName
                       . ".addChild(new LayerNode(\"" . $node->htmlName . "\"));"
                       . "\n";
                }
                else {
                    $node->writeJSString(0);
                    echo $this->htmlName . ".addChild(" . $node->htmlName . ");\n";
                }
            }
        }

        function & find($name) {
            ###### Note the use of reference when calling this method
            ######  $node = & $root->find("child_2_0");          ### <------------ Note reference
            if ($this->name == $name) return $this;

            if (! $this->children) return null;
            for ($i=0; $i<count($this->children); $i++) {
                $node = &$this->children[$i]->find($name);       ### <------------ Note reference
                if ($node) return $node;
            }
            return null;
        }

        function getDepth() {   ## Return the number of parent nodes (num nodes with children)
            if (!$this->children) return 0;
            $depth = 1;   ## 
            for ($i=0; $i<count($this->children); $i++) {
                $d = 1;
                $d += $this->children[$i]->getDepth();
                $depth = $d > $depth ? $d : $depth;
            }
            return $depth;
        }

        function buildWidget(&$tbl, $thisDepth, $totDepth, $relations) {
           
            ## We only capture rows for each child and not for self
            if (!$this->children || count($this->children) == 0) return;

            if (!$relations) {       ## This will only be undefined for the root node
                $relations = array();
                for ($j=0; $j<$totDepth; $j++) array_push($relations, DF_TREE_NULL);
            }

            ###$totDepth --; # Subtract 1 from total depth since we don't consider the root node
            #echo "thisDepth=" . $thisDepth . ", " . $this->name . ", totalDepth=" . $totDepth . "<br />\n";


            ## Process each child node
            for ($i=0; $i<count($this->children); $i++) {
#if ($this->name == "rootNode") echo "buildWidget: ".$this->name.", child=".$this->children[$i]->name." <br />\n";
#echo "Name=" . $this->children[$i]->name . ", num children=" . count($this->children) . ", i=" . $i . "<br />\n";
                unset($row);  ## I believe this will destroy the reference but leave the object intact
                unset($node);
                $node = &$this->children[$i];

                #$isLastChild = ( isset($node->children) && (intval($i+1) == count($node->children)) ? true : false);
                $isLastChild = $this->children && $i+1 == count($this->children) ? true : false;
##echo "Name=" . $this->children[$i]->name . ", isset=" . isset($node->children) . ", num children=" . count($this->children) . ", i=" . $i . ", last=" . ($isLastChild?"true":"false") . "<br />\n";

                $row = new RowWidget("r_ly_".$node->name, null, null, null, DF_LABEL_NONE, DF_BLOCK_NONE);
                $node->htmlName = $row->name;
                if ($node->type == "group") {

                    ## ## ## ## ###  Possible decendent lines in columns 0 to $thisDepth ## ## ## ## ##
                    for ($j=0; $j<$thisDepth; $j++) {
                        $dcndClass = getDecendentClass($relations[$j]);
                        $row->add (new TextWidget("tw_ly_0_".$j.$node->name,null,null, DF_BLOCK_NONE), $class);
                    }

                    ## Define ancestral relation to pass to next child
                    $childRelations = setChildRelations($isLastChild, $thisDepth, $relations);

#echo "depth=".$thisDepth." ".$this->name." ".$node->name; for ($j=0; $j<count($childRelations); $j++) { echo " ".$childRelations[$j]; } echo "<br />\n";
                    ## ## ## ## ###  ## #### ### ### ### ### ### ### ### ### ### ### ### ## ## ## ## ##

                    unset($inpImg);                    
                    $inpImg = new InputImage("img_".$node->name, null, "lgnd_icon", "Click to expand",DF_LABEL_NONE, DF_BLOCK_NONE,"imagedir/expanded_4.gif");
                    $inpImg->setJSEvent("onclick");
                    $inpImg->setJSMethod("expandCollapse('" . $row->name . "');return false;");
                    $node->imgName = $inpImg->name;
                    $row->add ($inpImg, "exp_cell");


                    for ($j=$thisDepth+1; $j<$totDepth-1; $j++) {
                        $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE));
                    }
 

                    $row->add (new TextWidget("tw_ly_2_".$node->name,null,null, DF_BLOCK_NONE));
                    $row->add (new TextWidget("tw_ly_3_".$node->name,null,null, DF_BLOCK_NONE));
                    $row->add (new TextWidget("tw_ly_4_".$node->name, $node->label, null, DF_BLOCK_NONE), "lgnd_group", 2);
                    $tbl->add($row, "rw_".$node->name);

                    #if ($i == count($this->children) - 1) $relations[$thisDepth+1] = DF_TREE_LASTCHILD;
                    #else $relations[$thisDepth+1] = DF_TREE_CHILD;
                    if ($isLastChild) $relations[$thisDepth] = DF_TREE_LASTCHILD;
                    else $relations[$thisDepth] = DF_TREE_CHILD;

                    #echo "num children=".count($node->children)."<br />";
                    $node->buildWidget($tbl, $thisDepth+1, $totDepth, $childRelations);   ## Recursive call to child node
                }
                else if ($node->type == "layer") {
                    ## Requires checkboxes

                    ## ### Layer has multiple classes  ###
                    if ($node->children && count($node->children) > 1) {  ## Requires icon to expand/collapse rows
                        for ($j=0; $j<$thisDepth; $j++) {
                            $dcndClass = getDecendentClass($relations[$j]);
                            $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE), $dcndClass);
                        }

                        unset($inpImg);                    
                        $inpImg = new InputImage("img_".$node->name, null, "lgnd_icon", "Click to expand",
                                      DF_LABEL_NONE, DF_BLOCK_NONE,"imagedir/expanded_4.gif");
                        $inpImg->setJSEvent("onclick");
                        $inpImg->setJSMethod("expandCollapse('" . $row->name . "');return false;");
                        $node->imgName = $inpImg->name;
                        $row->add ($inpImg, "exp_cell");


                        for ($j=$thisDepth+1; $j<$totDepth-1; $j++)
                            $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE));

                        $classLgnd = "lgnd_cmajor";
                    }
                    ## ### Layer has only one class ###
                    else {               #######   a one-class layer
                        ## ## ###  Possible decendent lines in columns 0 to $thisDepth ## ## ##
                        $childRelations = $relations;
                        if ($isLastChild && $thisDepth > 0) $childRelations[$thisDepth-1] = DF_TREE_LASTCHILD;

                        for ($j=0; $j<$thisDepth; $j++) {
                            $dcndClass = getDecendentClass($childRelations[$j]);
                            ##$dcndClass = getDecendentClass($relations[$j]);
                            $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE), $dcndClass);
                        }
                        for ($j=$thisDepth; $j<$totDepth-1; $j++)
                            $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE));

                        $classLgnd = "lgnd_major";
                    }

                    $chk1 = new CheckBox($node->name."_vi",null,"chk1", null, DF_LABEL_NONE, DF_BLOCK_NONE);
                    $chk2 = new CheckBox($node->name."_lb",null,"chk1", null, DF_LABEL_NONE, DF_BLOCK_NONE);

                    $row->add($chk1, "chk_cell");
                    $row->add($chk2, "chk_cell");

                    if (!$node->children || count($node->children) < 2) {
                        ##$iw = new ImageWidget("iw1_".$node->name, null, null, DF_BLOCK_NONE, "imagedir/linepat1.gif", 18,12, $node->label);
                        ## Name ImageWidget's for legend icons using layer/class index to retrieve them later and set the src file name
                        $iw = new ImageWidget("lgimg_".$node->layIdx."_".$node->clsIdx, null, null, DF_BLOCK_NONE, $node->layIdx."_".$node->clsIdx, 18,12, $node->label);
                        $row->add($iw, "lgn_icon");
                    }
                    else
                        $row->add (new TextWidget("tw_ly_3_".$node->name,null,null, DF_BLOCK_NONE));

                    $row->add (new TextWidget("tw_ly_4_".$node->name, $node->label,null, DF_BLOCK_NONE), $classLgnd);

                    $tbl->add($row, "rw_".$node->name);

                    ## ## ## ## ###  ## #### ### ### ### ### ### ### ### ### ### ### ### ## ## ## ## ##
                    ## Define ancestral relation to pass to next child
                    $childRelations = setChildRelations($isLastChild, $thisDepth, $relations);
                    ## ## ## ## ###  ## #### ### ### ### ### ### ### ### ### ### ### ### ## ## ## ## ##

                    $node->buildWidget($tbl, $thisDepth+1, $totDepth, $childRelations);   ## Recursive call to child node for classes
                }
                else {    ## type == "class"

                    if ($isLastChild) $relations[$thisDepth-1] = DF_TREE_LASTCHILD;
                    else $relations[$thisDepth-1] = DF_TREE_CHILD;

                    ## ## ###  Possible decendent lines in columns 0 to $thisDepth ## ## ##
                    for ($j=0; $j<$thisDepth; $j++) {
                        $dcndClass = getDecendentClass($relations[$j]);
                        $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE), $dcndClass);
                    }
                    for ($j=$thisDepth; $j<$totDepth-1; $j++)
                        $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE));

                    ## ## ###  Possible decendent lines in columns 0 to $thisDepth ## ## ##
                    #for ($j=0; $j<$totDepth-1; $j++)
                    #    $row->add (new TextWidget("tw_ly_1_".$j.$node->name,null,null, DF_BLOCK_NONE));

                    $row->add (new TextWidget("tw_ly_3_".$node->name,null,null, DF_BLOCK_NONE));
                    $row->add (new TextWidget("tw_ly_4_".$node->name,null,null, DF_BLOCK_NONE));

                    #$iw = new ImageWidget("iw1_".$node->name, null, "chk1", DF_BLOCK_NONE, "imagedir/coplat.gif", 18,12, $node->label);
                    $iw = new ImageWidget("lgimg_".$node->layIdx."_".$node->clsIdx, null, "chk1", DF_BLOCK_NONE, $node->layIdx."_".$node->clsIdx, 18,12, $node->label);
                    $row->add($iw, "lgn_icon");
                    $row->add (new TextWidget("tw_ly_6_".$node->name,$node->label,null, DF_BLOCK_NONE), "lgnd_minor");
                    $tbl->add($row, "rw_".$node->name);
                }
                #$childRelations = $relations;
                #for ($j=0; $j<$thisDepth; $j++) {
                #    if ($relations[$j] == DF_TREE_LASTCHILD) $childRelations[$j] = DF_TREE_NULL;
                #}
            }
        }
    }

    ## =================================================================================

    function getDecendentClass($relation) {
        if ($relation == DF_TREE_INDIRECT) return("dcnd_indirect");
        else if ($relation == DF_TREE_CHILD) return("dcnd_child");
        else if ($relation == DF_TREE_LASTCHILD) return("dcnd_last_child");
        else return null;
    }
    ## =================================================================================

    function setChildRelations($isLastChild, $thisDepth, $childRelations) {
        ## Define ancestral relation to pass to next child

        if ($isLastChild) $childRelations[$thisDepth] = DF_TREE_LASTCHILD;
        else $childRelations[$thisDepth] = DF_TREE_CHILD;
        for ($j=0; $j<$thisDepth; $j++) {
            if ($childRelations[$j] == DF_TREE_LASTCHILD)
                $childRelations[$j] = DF_TREE_NULL;
            
            else if ($childRelations[$j] == DF_TREE_CHILD || $childRelations[$j] == DF_TREE_LASTCHILD)
                $childRelations[$j] = DF_TREE_INDIRECT;
        }
        return $childRelations;
    }

?>
