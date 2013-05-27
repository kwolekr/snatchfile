snatchfile v1.0

================

 NAME
    snatchfile -- watch a directory for files being written and snatch them

 SYNOPSIS
    snatchfile [watch_dir] [-c copy_dir] [-e command_string] [-h] [-m match_string] [-o] [-s]

 DESCRIPTION
    The snatchfile utility either copies or executes a user-specified command when a write
    occurs in a directory or subdirectory being watched.  If a directory to watch is not
    specified, the current working directory is watched.

    The options are as follows:

    -c     Copy directory.  Directory to which snatched files are copied to.  By default, files
           are copied to the current working directory.

    -e     Command string.  Command to be executed via ShellExecute's "open" verb when a matching
           file in the directory is modified, with the sole parameter being the full filename of
           the changed file. If a command string to execute is not specified, the file written to
           is copied to the copy directory.

    -h     Display usage and exit.

    -m     Match string.  If a filename matches this wildcard expression, the action (copy or
           exec command) will be performed.  If no match string is specified, no files will be
           filtered, and the action will be performed for any file in the watch directory that is
           changed.

    -o     Do not overwrite.  Without the -o option, any previously existing file in the copy
           directory will be overwritten if a file of the same name is snatched.

    -s     Watch subdirectories.  Subdirectories of the watch directory will also be monitored.

 EXIT STATUS
    The snatchfile utility exits 0 on success, and >0 if an error occurs.

 RATIONALE
    Malware may create temporary files and delete them soon after, leaving no simple way to examime
    them.  Debugging may be unfeasable, and previously creating the file with a read-only attribute
    may not work either.  This utility 'snatches' the file before it can be deleted, and copies it
    to some other location for later analysis.  Of course, there could be many other uses for this
    utility not yet realized.

 BUGS
    The match string is not a full-fledged regular expression, only a wildcard.
    Long filenames (prefixed with "\\?\") are not supported.  Due to poor support for long filenames
    in general, malware may take advantage of this to make analysis slightly more difficult.
    Only changes to files in the directory being watched are reported.  An option to act on other
    events should exist too, and perhaps pass what event happened to the command being executed too.
    This utility utilizes the ReadDirectoryChangesW routine, which only reports changes to the
    directory, not the actual files within.  Therefore, a program could subvert a write being
    reported by this utility by calling SetFileTime with 0xFFFFFFFF FFFFFFFF to prevent filetimes
    from being changed.  To remedy such a problem, this utility could use change journals or even a
    filesystem filter driver to track changes.
