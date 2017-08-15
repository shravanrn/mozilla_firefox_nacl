My notes
========
Version of firefox modified to use a sandboxed version of libjpeg. Update the paths in media/libjpeg_naclport/moz.build and then build with ./mach build.

To run use
# for the purposes of testing we are turning of seccomp. Eventually, we should perform all initialization prior to seccomp init
export MOZ_DISABLE_CONTENT_SANDBOX=1
./mach run

The sandboxed version of libjpeg is compiled with NaCl's gcc compiler. 
This binary is then loaded with the help of the library from https://github.com/shravanrn/Sandboxing_NaCl.git

Note if you are building the Sandoxing_NaCl library from scratch, make sure to use settings compatible with firefox compile settings.
To do this, replace the SConstruct file with SConstruct_Firefox and the built it

/////////////////////////////////////////////////////////////////////////////////////


An explanation of the Mozilla Source Code Directory Structure and links to
project pages with documentation can be found at:

    https://developer.mozilla.org/en/Mozilla_Source_Code_Directory_Structure

For information on how to build Mozilla from the source code, see:

    https://developer.mozilla.org/en/docs/Build_Documentation

To have your bug fix / feature added to Mozilla, you should create a patch and
submit it to Bugzilla (https://bugzilla.mozilla.org). Instructions are at:

    https://developer.mozilla.org/en/docs/Creating_a_patch
    https://developer.mozilla.org/en/docs/Getting_your_patch_in_the_tree

If you have a question about developing Mozilla, and can't find the solution
on https://developer.mozilla.org, you can try asking your question in a
mozilla.* Usenet group, or on IRC at irc.mozilla.org. [The Mozilla news groups
are accessible on Google Groups, or news.mozilla.org with a NNTP reader.]

You can download nightly development builds from the Mozilla FTP server.
Keep in mind that nightly builds, which are used by Mozilla developers for
testing, may be buggy. Firefox nightlies, for example, can be found at:

    https://archive.mozilla.org/pub/firefox/nightly/latest-mozilla-central/
            - or -
    https://nightly.mozilla.org/
