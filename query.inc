<?php
### ==========================================================================
###  $Id: query.inc,v 1.1.1.1 2007/04/24 02:04:19 rob Exp $
###  $Name:  $
### ==========================================================================

    ## Global function to draw a individual image for one seismic line

    function drawLineImg($map, $objShp) {

        ## Create a new map object for this individual shape
        ## Clone only what we need from the master map object
        $newMap = ms_newMapObj("seis_dummy.map");
        $newMap->setSymbolSet( $map->symbolsetfilename );
        $newMap->setFontSet( $map->fontsetfilename );
        $newMap->setSize($map->width, $map->height);
        $newMap->setExtent( $map->extent->minx, $map->extent->miny,
                            $map->extent->maxx, $map->extent->maxy );
        $newMap->setProjection($map->getProjection());
        $newMap->web->set("imagepath", $map->web->imagepath);
        $newMap->web->set("imageurl", $map->web->imageurl);

        #Clone output format object
        $newMap->outputformat->mimetype    = $map->outputformat->mimetype;
        $newMap->outputformat->driver      = $map->outputformat->driver;
        $newMap->outputformat->extension   = $map->outputformat->extension;
        $newMap->outputformat->renderer    = $map->outputformat->renderer;
        $newMap->outputformat->imagemode   = $map->outputformat->imagemode;
        $newMap->outputformat->transparent = $map->outputformat->transparent;

        if ($objShp->type == MS_SHAPE_LINE)
            $layer = $newMap->getLayerByName("cseis");
        else if ($objShp->type == MS_SHAPE_POINT)
            $layer = $newMap->getLayerByName("samples");
        else
            return null;

        
        $img = $newMap->prepareImage();
        $objShp->draw($newMap, $layer, $img);
        ###$newMap->drawLabelCache($img);
        $url = $img->saveWebImage();
        $img->free();
        ##echo "<!--  url =  " . $url . "   -->\n";
        return $url;
    }

    ## ----------------------------------------------------------------------

    class QuerySet {
        var $qExtent;       ## Extents of query window in world coordinates
                            ## when selecting a query area or 3D window
                            ## using rectangle search area
        var $isActive;      ## indicates the the query window has been defined
        var $objLayer = null;
        var $type = 'point';
        var $imgOverlays;   ## Contains list of image urls for overlay images
        var $maxPrld = 50;  ## Maximum number of preloaded images to generate

        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        function QuerySet() {
            $this->isActive = false;
            $this->imgOverlays = array();
        }
        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        function isActivated() { return $this->isActive; }
        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        function setExtent(&$rect) {
            $this->type = 'rect';
            $this->isActive = true;
            $this->qExtent = ms_newRectObj();
            $this->qExtent->setExtent($rect->minx, $rect->miny,
                        $rect->maxx, $rect->maxy);
        }
        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        function getExtent() { return $this->qExtent; }
        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        function drawRect(&$map, &$image) {
            if ($this->type != 'rect') { return; }
            ##  -   -   -   -   -   -   -   -   -   -   -   -   -   -
            ## Draw selection area (Query window)

            ##$hltLayer = ms_newLayerObj($map);
            if (! ($hltLayer = $map->getLayerByName("highlight")) ) {
                echo "Layer not found<br />\n";
                return;
            }
            $hltLayer->set("status",1);

            ##$shp = ms_newShapeObj(MS_SHAPE_POLYGON);
            ##$shp->add($this->qExtent);
            $this->qExtent->draw($map, $hltLayer, $image, 0, "Test");
                
            ##  -   -   -   -   -   -   -   -   -   -   -   -   -   -
        }   


        function setLayer ($olayer) { $this->objLayer = $olayer; $this->isActive = true; }

        function getNumResults() {
            if (! $this->objLayer) return null;
            return $this->objLayer->getNumResults();
        }

        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        function crOverlays(&$map, &$errmsgs) {
            if (! $this->isActive ) return; #Don't write any thing if  no query

            if ( ! $this->objLayer ) return;
                ##array_push($errmsgs, "No results returned from query (Object layer not defined)");

            elseif ( ($numResults=$this->objLayer->getNumResults() ) == 0) {
                $items =  $this->objLayer->getItems();
                array_push($errmsgs, 
                    "No results returned from query (" . $numResults . ") num items = " . sizeof($items));
            }
            elseif ( $numResults > 0 && $this->objLayer->open() != MS_SUCCESS) {
                array_push($errmsgs, "No results returned from query (Failed to open layer)");
            }
            else {
                ## Arbitrarily set max  number of preloaded images. Need to be nice to our browsers!
                ## If number of results exceeds the max, then issue warning message
                $numToPrld = $numResults < $this->maxPrld ? $numResults : $this->maxPrld;
                if ($numResults > $this->maxPrld) {
                    array_push($errmsgs,
                        "Warning: mouseover will work with up to " . $this->maxPrld . " seismic lines");
                }
##echo $this->objLayer->type;

                for ($j=0; $j<$numToPrld; $j++) {   ## Do for each image to preload
                    $objRes = $this->objLayer->getResult($j);
                    $objShp = $this->objLayer->getShape($objRes->tileindex, $objRes->shapeindex);

                    ##
                    ##  Generate a transparent overlay image of this one line
                    $urlImg = drawLineImg($map, $objShp);

                    array_push($this->imgOverlays, 
                        preg_replace('/.*\/([0-9a-zA-z_]+)\.[A-Z,a-z]+/','\1',$urlImg));

                    $objShp->free();
                }
                $this->objLayer->close();
            }
        }

        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        function htmlWritePreload() {
            for ($i=0; $i< count($this->imgOverlays); $i++) {
                if ($i > 0 && $i % 10 == 0) echo "\n";
                if ($i > 0) echo ",";
                echo "'" . $this->imgOverlays[$i] . "'";
            }
        }
        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        function htmlWrite(&$map) {
            if (! $this->isActive ) return; #Don't write any thing if  no query


            if ( ! $this->objLayer ) {
                echo "No results returned from query (Object layer not defined)\n";
            }
            elseif ( ($numResults=$this->objLayer->getNumResults() ) == 0) {
                echo "No results returned from query (" . $numResults . ")\n";
                $items =  $this->objLayer->getItems();
                echo "<br />num items = " . sizeof($items) . "\n";
            }
            elseif ( $numResults > 0 && $this->objLayer->open() != MS_SUCCESS) {
                echo "No results returned from query (Failed to open layer)\n";
            }
            else {
                $tablename = preg_replace('/.* from (\w+) .*/', '$1', strtolower($this->objLayer->data));
                ##echo $tablename . "<br />\n";
                ## Get list of columns to be displayed
                $cols = split(',', $this->objLayer->getMetaData('resultcols'));

                ## Write column heading
                $items =  $this->objLayer->getItems();
                echo "<table border=\"0\" cellspacing=\"0\" cellpadding=\"2\" align=\"center\"><tr>\n";
                for ($j=0; $j<count($cols); $j++)
                    echo "<td>" . ucwords(preg_replace('/_/',' ',$cols[$j])) . "</td>";
                echo "</tr>\n";

                ##$str .=  "numResults=".$numResults."<br />\n";
                ##$maxPreloads = count($this->imgOverlays);  ## max num of images to preload in javascript
                for ($j=0; $j<$numResults; $j++) {
                    $objRes = $this->objLayer->getResult($j);
                    $objShp = $this->objLayer->getShape($objRes->tileindex, $objRes->shapeindex);

                    ##
                    ##  Generate a transparent overlay image of this one line
                    ##$urlImg = drawLineImg($map, $objShp);

                    $rowClass = $j%2 == 0 ? 'row_qset_e' : 'row_qset_o';  ## Alternate the background style
                    echo "<tr class=\"" . $rowClass . "\">";

                    for ($k=0; $k<count($cols); $k++) {
                        echo "<td>";

                        ## If first one in row, we need beginning <a tag
                        if ( $k == 0) {
                            echo "<a href=\"#\"";
                            if ($j < $this->maxPrld) {
                                echo " onMouseOver=\"swapToHlt('mainmap'," . $j . ");\""
                                   . " onMouseOut=\"swapToBase('mainmap');\"";
                            }
                            echo " onclick=\"opDetailWindow('" . $tablename . "','"
                                . $objShp->values[$cols[$k]]
                                . "');return false;\">";
                        }
                        echo $objShp->values[$cols[$k]];
                        if ( $k == 0) echo "</a>";
                        echo "</td>";
                    }

                    echo "</tr>\n";

                    $objShp->free();
                }
                echo "</table>\n";

                $this->objLayer->close();
            }
        }

        ## - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    }
            
## ----------------------------------------------------------------------
?>
