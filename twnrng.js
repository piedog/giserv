//                      | T North | T South | R East  | R West   |
//  meridCode, meridName, min, max, min, max, min, max, min, max
 var twnRng = Array();
 twnRng["01"]=new Array(1,19,1,8,1,18,1,4); twnRng["02"]=new Array(1,38,1,9,1,15,1,15); twnRng["03"]=new Array(1,46,1,17,1,15,1,14);
 twnRng["04"]=new Array(1,29,1,14,1,11,1,10); twnRng["05"]=new Array(1,168,1,20,1,18,1,107); twnRng["06"]=new Array(1,58,1,35,1,25,1,121);
 twnRng["07"]=new Array(1,23,1,12,1,34,0,0); twnRng["08"]=new Array(1,65,1,16,1,46,1,7); twnRng["09"]=new Array(0,0,1,18,1,11,1,13);
 twnRng["10"]=new Array(1,31,0,0,1,19,1,10); twnRng["11"]=new Array(1,6,0,0,1,28,0,0); twnRng["12"]=new Array(1,28,1,83,1,102,1,11);
 twnRng["13"]=new Array(1,37,1,22,1,34,1,28); twnRng["14"]=new Array(1,42,1,24,1,32,1,25); twnRng["15"]=new Array(1,19,1,5,1,8,1,3);
 twnRng["16"]=new Array(0,0,1,22,1,14,1,19); twnRng["17"]=new Array(1,29,1,10,1,27,1,27); twnRng["18"]=new Array(1,23,1,24,1,32,1,16);
 twnRng["19"]=new Array(1,67,1,10,1,17,1,49); twnRng["20"]=new Array(1,37,1,16,1,63,1,35);
 twnRng["21"]=new Array(1,48,1,33,1,71,1,19); twnRng["22"]=new Array(1,7,0,0,0,0,5,11);
 twnRng["23"]=new Array(1,51,1,34,1,39,1,22); twnRng["24"]=new Array(0,0,1,20,1,20,1,5); twnRng["25"]=new Array(1,24,1,10,1,31,1,22);
 twnRng["26"]=new Array(1,15,1,44,1,26,1,20); twnRng["27"]=new Array(1,31,1,19,1,27,1,36); twnRng["28"]=new Array(1,34,1,103,1,15,1,266);
 twnRng["29"]=new Array(1,7,1,68,1,43,1,34); twnRng["30"]=new Array(1,5,1,7,1,3,1,12); twnRng["31"]=new Array(1,2,1,4,1,3,1,3);
 twnRng["32"]=new Array(1,18,0,0,1,14,1,5); twnRng["33"]=new Array(1,41,1,41,1,51,1,16); twnRng["34"]=new Array(1,9,1,2,1,6,1,6);
 twnRng["37"]=new Array(1,2,0,0,0,0,10,10); twnRng["39"]=new Array(1,4,0,0,0,0,22,22); twnRng["40"]=new Array(2,5,0,0,0,0,22,22);
 twnRng["41"]=new Array(1,1,0,0,0,0,23,23); twnRng["43"]=new Array(1,4,0,0,0,0,0,0); twnRng["44"]=new Array(1,34,1,29,1,31,1,68);
 twnRng["45"]=new Array(1,24,1,17,1,48,1,63); twnRng["46"]=new Array(1,71,0,0,1,30,1,32); twnRng["47"]=new Array(1,15,0,0,1,8,0,0);
 twnRng["35"]=new Array(1,10,0,0,0,0,1,20);


        function TwnRngObj( TorR, txr_range, txt_value, txr_msg, hid_value, rb_dir1, rb_dir2, sl_meridians) {
            this.TorR = TorR;
            this.txr_range = getObject(txr_range);
            this.txt_value = getObject(txt_value);
            this.txr_msg = getObject(txr_msg);
            this.hid_value = getObject(hid_value);
            this.rb_dir1 = getObject(rb_dir1);
            this.rb_dir2 = getObject(rb_dir2);
            this.sl_meridians = getObject(sl_meridians);
        }

        TwnRngObj.prototype.getMin = function(rb) {   // ibtn = { 0=rb_dir1, 1=rb_dir2 }
            var ibtn = rb == this.rb_dir1 ? 0 : 1;
            var mercode = this.sl_meridians.options[this.sl_meridians.selectedIndex].value;
            var value = twnRng[mercode][this.TorR*4+ibtn*2];
            return value;
        }

        TwnRngObj.prototype.getMax = function(rb) {   // ibtn = { 0=rb_dir1, 1=rb_dir2 }
            var ibtn = rb == this.rb_dir1 ? 0 : 1;
            var mercode = this.sl_meridians.options[this.sl_meridians.selectedIndex].value;
            var value = twnRng[mercode][this.TorR*4+ibtn*2+1];
            return value;
        }

        TwnRngObj.prototype.init = function(mercode) {
            this.sl_meridians.selectedIndex = 0;  // default
            if (mercode) {
                var i=0;
                while (i<this.sl_meridians.options.length && this.sl_meridians.options[i].value != mercode) { i++; }
                if (i < this.sl_meridians.options.length) this.sl_meridians.selectedIndex = i;
            }
            
            if (!this.rb_dir1.checked && !this.rb_dir2.checked) this.rb_dir2.checked = true;

            this.setButtons();
            this.setRangeMsg();
            if (!this.txt_value.value || this.txt_value.value == "")
                this.txt_value.value = this.getMin( this.rb_dir1.checked ? this.rb_dir1 : this.rb_dir2 );
            this.validate();
        }

        TwnRngObj.prototype.setButtons = function() {
            var min1 = this.getMin(this.rb_dir1);
            var min2 = this.getMin(this.rb_dir2);
            this.rb_dir1.disable = false;
            this.rb_dir2.disable = false;
            if (min1 == 0 || min2 == 0) {  // Disable direction buttons
                this.rb_dir1.disable = true;
                this.rb_dir2.disable = true;
                if (min1 == 0) {
                    this.rb_dir1.checked = false;
                    this.rb_dir2.checked = true;
                }
                else {
                    this.rb_dir1.checked = true;
                    this.rb_dir2.checked = false;
                }
            }
        }

        TwnRngObj.prototype.setRangeMsg = function() {
            if (this.rb_dir1.checked) this.txr_range.value = "("+this.getMin(this.rb_dir1)+","+this.getMax(this.rb_dir1)+")";
            else this.txr_range.value = "("+this.getMin(this.rb_dir2)+","+this.getMax(this.rb_dir2)+")";
        }


        TwnRngObj.prototype.update = function() {
            this.setButtons();
            this.setRangeMsg();
            this.validate();
        }

        TwnRngObj.prototype.validate = function() {
            var btnchk = this.rb_dir1.checked ? this.rb_dir1 : this.rb_dir2;
            var min = this.getMin(btnchk);
            var max = this.getMax(btnchk);
            this.txr_msg.value = "";
            if (! this.txt_value || this.txt_value.value == "")
                this.txr_msg.value = "Value required";
            else if (min > this.txt_value.value || max < this.txt_value.value)
                this.txr_msg.value = "Value out of range";
            else
                this.hid_value = this.txt_value.value;
        }
