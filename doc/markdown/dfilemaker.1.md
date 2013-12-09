% DFILEMAKER(1)

# NAME

dfilemaker - distributed random file generation program

# SYNOPSIS

dfilemaker [options] path

# DESCRIPTION
dfilemaker is a tool for generating files and file trees which contain files
suitable for testing.

# OPTIONS

-d, \--debug=*level*
:   Specify the level of debug information to output. Level may be one of:
    *fatal*, *err*, *warn*, *info*, or *dbg*. Increasingly verbose debug
    levels include the output of less verbose debug levels.

-e, \--depth=*min*-*max*
:   Specify the depth of the filesystem tree to generate. The depth will be
    selected at random within the bounds of min and max. The default depth
    is set to 10 min, 20 max.

-f, \--fill=*type*
:   Specify the fill pattern of the file. Current options available are:
    `random`, `true`, `false`, and `alternate`. `random` will fill the file
    using urandom(4). `true` will fill the file with a 0xFF pattern. `false`
    will fill the file with a 0x00 pattern. `alternate` will fill the file
    with a 0xAA pattern. The default fill is `random`.

-h, \--help
:   Print a brief message listing the *dfilemaker(1)* options and usage.

-i, \--seed=*integer*
:   Specify the seed to use for random number generation. This can be used to
    create reproducible test runs. The default is to generate a random seed.

-r, \--ratio=*min*-*max*
:   Specify the ratio of files to directories as a percentage. The ratio will
    be chosen at random within the bounds of min and max. The default ratio
    is 5% min to 20% max.

-s, \--size=*min*-*max*
:   Specify the file sizes to generate. The file size will be chosen at random
    random within the bounds of min and max. The default file size is set from
    1MB to 5MB.

-v, \--version
:   Print version information and exit.

-w, \--width=*min*-*max*
:   Specify the width of the filesystem tree to generate. The width will be
    selected at random within the bounds of min and max. The width of the
    tree is determined by counting directories. The default width is set to
    10 min, 20 max.

# SEE ALSO

`dcmp` (1).
`dcp` (1).
`dfilemaker` (1).
`dfind` (1).
`dgrep` (1).
`dparallel` (1).
`drm` (1).
`dtar` (1).
`dwalk` (1).

The FileUtils source code and all documentation may be downloaded from
<http://fileutils.io>
