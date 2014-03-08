import os
import ycm_core

flags = [
        '-Wall',
        '-Wextra',
        '-Werror',
        '-std=c++11',
        '-I', '.',
        '-I', '/usr/include/eigen3'
        ]

def DirectoryOfThisScript():
    return os.path.dirname( os.path.abspath( __file__ ) )

def MakeRelativePathsInFlagsAbsolute( flags, working_directory ):
    if not working_directory:
        return list( flags )
    new_flags = []
    make_next_absolute = False
    path_flags = [ '-isystem', '-I', '-iquote', '--sysroot=' ]
    for flag in flags:
        new_flag = flag

        if make_next_absolute:
            make_next_absolute = False
            if not flag.startswith( '/' ):
                new_flag = os.path.join( working_directory, flag )

        for path_flag in path_flags:
            if flag == path_flag:
                make_next_absolute = True
                break

            if flag.startswith( path_flag ):
                path = flag[ len( path_flag ): ]
                new_flag = path_flag + os.path.join( working_directory, path )
                break

        if new_flag:
            new_flags.append( new_flag )
    return new_flags


def FlagsForFile( filename, **kwargs ):
    final_flags = MakeRelativePathsInFlagsAbsolute( flags, DirectoryOfThisScript() )
    return {
            'flags': final_flags,
            'do_cache': True
            }
