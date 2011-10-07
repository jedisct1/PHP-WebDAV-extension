<?php #$Id: dav_stream.inc.php 499 2007-04-05 19:14:37Z j $

class DavStream {
    var $fd = NULL;
    var $path = NULL;
    var $mode = 0;
    var $errors = TRUE;
    var $content = NULL;
    var $content_size = 0;
    var $blksize = 100000;
    var $offset = 0;

    static $timeout = 5;
    static $user_name = '';
    static $user_password = '';

    function set_timeout($timeout) {
        self::$timeout = $timeout;
    }

    function get_timeout() {
        return self::$timeout;
    }

    function set_user_name($user_name) {
        self::$user_name = $user_name;
    }

    function get_user_name() {
        return self::$user_name;
    }

    function set_user_password($user_password) {
        self::$user_password = $user_password;
    }

    function get_user_password() {
        return self::$user_password;
    }

    function read_content() {
        if ($this->content !== NULL) {
            return TRUE;
        }
        if ($this->fd === NULL) {
            return FALSE;
        }
        $ret = webdav_get('', $this->fd);
        $this->content = $ret;
        $this->content_size = strlen($this->content);

        return TRUE;
    }

    function stream_open($path, $mode, $options, &$opened) {
        $this->path = $path;
        if (strchr($mode, 'r') !== FALSE) {
            $this->mode = 'read';
        } elseif (strchr($mode, 'w') !== FALSE ||
                  strchr($mode, 'a') !== FALSE) {
            $this->mode = 'write';
        } else {
            return FALSE;
        }
        if ($options & STREAM_USE_PATH) {
            $opened = $path;
        }
        if ($options & STREAM_REPORT_ERRORS) {
            $this->errors = TRUE;
        } else {
            $this->errors = FALSE;
        }
        $this->content = NULL;
        $this->fd = webdav_connect($path,
                                   self::$user_name, self::$user_password);

        return TRUE;
    }

    function stream_close() {
        $ret = webdav_close($this->fd);
        $this->fd = NULL;
        $this->content = NULL;
        $this->content_size = 0;
        $this->offset = 0;
        $this->mode = 0;

        return $ret;
    }

    function stream_stat() {
        if ($this->read_content() !== TRUE) {
            return FALSE;
        }
        $now = time();
        return array('dev' => 0,
                     'ino' => md5($this->path),
                     'mode' => 0644,
                     'uid' => 0,
                     'gid' => 0,
                     'rdev' => 0,
                     'size' => $this->content_size,
                     'atime' => $now,
                     'mtime' => $now,
                     'ctime' => $now,
                     'blksize' => $this->blksize,
                     'blocks' => ceil($this->content_size / $this->blksize)
                     );
    }

    function stream_read($count) {
        if ($this->offset > $this->content_size) {
            die('$this->offset > $this->content_size' . "\n");
        }
        $left = $this->content_size - $this->offset;
        if ($count > $left) {
            $count = $left;
        }
        if ($count <= 0) {
            return FALSE;
        }
        $ret = substr($this->content, $this->offset, $count);
        $this->offset += $count;

        return $ret;
    }

    function stream_write($data) {
        if ($this->mode !== 'write') {
            return FALSE;
        }
        webdav_put('', $data, $this->fd);

        return strlen($data);
    }

    function stream_eof() {
        if ($this->offset >= $this->content_size) {
            return TRUE;
        }
        return FALSE;
    }

    function stream_tell() {
        return $this->offset;
    }

    function stream_seek($offset, $whence) {
        $old_offset = $this->offset;

        switch ($whence) {
         case SEEK_SET:
            $this->offset = $offset;
            break;
         case SEEK_CUR:
            if ($offset < 0 && $offset > $this->offset) {
                $this->offset = 0;
            } else {
                $this->offset += $offset;
            }
            break;
         case SEEK_END:
            if ($offset > $this->content_size) {
                $this->offset = 0;
            } else {
                $this->offset = $this->content_size - $offset;
            }
        }
        if ($this->offset !== $old_offset) {
            return FALSE;
        }
        return TRUE;
    }

    function stream_flush() {
        return TRUE;
    }

    function rename($from, $to) {
        if (($fd = webdav_open
             ($path, self::$user_name, self::$user_password)) === FALSE) {
            return FALSE;
        }
        $ret = webdav_mkdir($from, $to, $fd);
        webdav_close($fd);

        return $ret;
    }

    function mkdir($path, $options) {
        if (($fd = webdav_open
             ($path, self::$user_name, self::$user_password)) === FALSE) {
            return FALSE;
        }
        $ret = webdav_mkdir($path, $fd);
        webdav_close($fd);

        return $ret;
    }

    function unlink($path) {
        if (($fd = webdav_open
             ($path, self::$user_name, self::$user_password)) === FALSE) {
            return FALSE;
        }
        $ret = webdav_unlink($path, $fd);
        webdav_close($fd);

        return $ret;
    }

    function url_stat($path, $options) {
        if (($fd = webdav_open
             ($path, self::$user_name, self::$user_password)) === FALSE) {
            return FALSE;
        }
        $content = webdav_get('', $fd);
        $ret = TRUE;
        if (empty($content)) {
            $ret = FALSE;
        }
        unset($content);
        webdav_close($fd);

        return $ret;
    }
}

stream_wrapper_register('webdav', 'DavStream');

?>
