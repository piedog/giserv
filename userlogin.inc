<?php
##===============================================================================
    require_once 'DB.php';

    ##===============================================================================
    class UserLogin {
        ## The php function session_start() must be called before instaniating
        ## this class.

        var $errMsg     = null;
        var $db         = null;
        var $password   = null;
        var $username   = null;
        var $defconn    = null;    ## Default connection for user nobody

        ## Host name and database name will be constant
        var $dbhost = 'redbud';    ## default host
        var $dbname = 'geotest';   ## default database name
        ##---------------------------------------------------------------------------
        function UserLogin() {
            ## Initialize
            ## Set up the default connection string for user nobody
            session_start();
            ## Define dbname and host from env vars if they've been set
            if (isset($_SERVER['GISERV_DBHOST']))
                $this->dbhost = $_SERVER['GISERV_DBHOST'];
            if (isset($_SERVER['GISERV_DBNAME']))
                $this->dbname = $_SERVER['GISERV_DBNAME'];
            $db_user = 'nobody';
            $db_pass = 'nobody';
            $this->defconn = "pgsql://" . $db_user . ":"
                           . $db_pass . "@" . $this->dbhost . " /" . $this->dbname;
        }

        ##---------------------------------------------------------------------------

        function validateUser($user, $pass) {
            ## Authenticate user from login screen

            ## Initial connection, use the username and password and try and login.
            ## If this fails, then the wrong combination of user/passw were given.
            $conn = "pgsql://" . $user . ":" . $pass . "@" . $this->dbhost . " /" . $this->dbname;
            PEAR::setErrorHandling(PEAR_ERROR_RETURN);
            $db = @DB::connect($conn);
            if (DB::isError($db)) {
                # This user/passw combination failed, return to caller
                $this->errMsg = 'could not connect, invalid usename, password';
                     ##echo $db->getMessage();
                return null;
            }
            ## Successful login, now disconnect and login using the web user name
            $db->disconnect();

            PEAR::setErrorHandling(PEAR_ERROR_RETURN);
            $db = @DB::connect($this->defconn);
            if (DB::isError($db)) {
                $this->errMsg = 'could not connect';
                return null;
            }
            $db->setFetchMode(DB_FETCHMODE_ASSOC);
            ## Store user name and passwords in cookie and session variables

            $sql = "select count(*) as nrows from web_users where session_id="
                 . $db->quote($_COOKIE['PHPSESSID']);
            $result = $db->getRow($sql);
            if (DB::isError($db)) { echo "Error, (getRow)<br>\n"; return null; }

            if ($result['nrows'] == 1) {
                $sql = "update web_users set"
                      . " password="   . $db->quote($pass)
                      . ",ip="         . $db->quote($_SERVER['REMOTE_ADDR'])
                      . ",exp_date="   . $db->quote(gmdate("Y-m-d"))
                      . " where session_id=" . $db->quote($_COOKIE['PHPSESSID']);

                $result = $db->query($sql);
                if (DB::isError($db)) { echo "Error, (query, update)<br>\n"; return null; }
            }
            else {  ## This session id no in table, insert a new one
                $sql = "insert into web_users (session_id,password,ip,exp_date) values("
                            . $db->quote($_COOKIE['PHPSESSID'])
                      . "," . $db->quote($pass)
                      . "," . $db->quote($_SERVER['REMOTE_ADDR'])
                      . "," . $db->quote(gmdate("Y-m-d"))
                      . ")";
                $result = $db->query($sql);
                if (DB::isError($result)) syslog(LOG_WARNING, $result->getMessage());
                if (DB::isError($result)) { echo "Error, (query, insert)<br>\n"; return null; }
            }
            $db->disconnect();
            #setcookie('giservLogin', $user,  time() + 31104000, '/directory/');
            setcookie('giservLogin', $user);
           
            return true;
        }

        ##---------------------------------------------------------------------------
        function checkConnection() {

            ## Check if there is a current session for this user. Checks if 
            ## there is a session in web_users table with this session id.
            ## If so, then downloads the password from web_users.
            ## Then, get the user name from the giservLogin cookie.
            ## If either of these 2 checks fail, then return null value.
            ## If user is valid, then return true.

            # Get the session variable
            if (!isset($_COOKIE['giservLogin'])) {
                $this->errMsg = 'checkConnection(): cookie error';
                return null;
            }

            if (!isset($_COOKIE['PHPSESSID'])) {
                $this->errMsg = 'checkConnection(): session error';
                return null;
            }

            # Get the password from web_users table
            PEAR::setErrorHandling(PEAR_ERROR_RETURN);
            $db = @DB::connect($this->defconn);
            if (DB::isError($db)) {
                # This user/passw combination failed, return to caller
                $this->errMsg = 'checkConnection(): connection error';
                return null;
            }

            $db->setFetchMode(DB_FETCHMODE_ASSOC);
            $sql = "select password from web_users where session_id="
                 . $db->quote($_COOKIE['PHPSESSID']);
            $result = $db->query($sql);
            if (DB::isError($result)) {
                $this->errMsg = 'checkConnection(): query error, ' . $result->getMessage();
                return null;
            }
            $result->fetchInto($row);
            
            if ($result->numRows() != 1) {
                $this->errMsg = 'checkConnection(): session not found in database';
                return null;
            }
            $this->password = $row['password'];
            $result->free();
            $db->disconnect();


            ## Now do the real connection with the real username and password
            $this->username = $_COOKIE['giservLogin'];

            $conn           = "pgsql://" . $this->username . ":"
                            . $this->password . "@" . $this->dbhost . "/" . $this->dbname;
            $db = DB::connect($conn);
            if (DB::isError($db)) {
                # This user/passw combination failed, return to caller
                #$this->errMsg = 'checkLoginStatus(): could not connect, invalid usename, password';
                $this->errMsg = 'checkLoginStatus(): could not connect, invalid usename('
                              . $this->username . '), password('.$this->password . ')'
                              . "<br>\n" . $conn;
                return null;
            }

            $db->setFetchMode(DB_FETCHMODE_ASSOC);
            $this->db = $db;
            return $db;
        }

        ##---------------------------------------------------------------------------
        function destroy() {
            $this->db->disconnect();
        }

        ##---------------------------------------------------------------------------

        function logout() {
            # Delete the user's session id from the web_users table
            PEAR::setErrorHandling(PEAR_ERROR_RETURN);
            $db = @DB::connect($this->defconn);
            if (DB::isError($db)) {
                # This user/passw combination failed, return to caller
                $this->errMsg = 'checkConnection(): connection error';
                return null;
            }

            $db->setFetchMode(DB_FETCHMODE_ASSOC);
            $sql = "delete from web_users where session_id="
                 . $db->quote($_COOKIE['PHPSESSID']);
            $result = $db->query($sql);
            if (DB::isError($result)) {
                $this->errMsg = 'checkConnection(): query error, ' . $result->getMessage();
                return null;
            }
            
            $db->disconnect();
            $rtn = session_destroy();
        }

        ##---------------------------------------------------------------------------
        function getUsername() { return $this->username; }
        function getPassword() { return $this->password; }
        function getErrorMsg() { return $this->errMsg;   }
        function getStatus()   { return $this->db ? true : false; }
        function & getConnect()  { return $this->db;       }
        function getDBHost()   { return $this->dbhost;   }
        function getDBName()   { return $this->dbname;   }

        ##---------------------------------------------------------------------------

    }
    ##===============================================================================
