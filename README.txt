
Apache Solr Attachments for 6.x

Requires the ability to run java and installation of tika 0.3 or higher.

see:  
http://lucene.apache.org/tika/gettingstarted.html
http://lucene.apache.org/tika/formats.html

Tika will extra many file formats, including PDFs, MS Office (2003 format
as well as new docx format).  Java 6 (aka 1.6) may be needed on some
platforms to support all formats.  The page on formats seems not to be 100% 
up to date.  In particular, https://issues.apache.org/jira/browse/TIKA-152
is committed, so it does currently support MS Office 2007 documents to 
some reasonable degee.

The easiest-to-find pre-built tika is likely a Solr nightly such as:

http://people.apache.org/builds/lucene/solr/nightly

Tika is located here at apachesolr-nightly/example/solr/lib.  You can copy/move
directory to somewhere convenient, though it's probably a good idea
to keep it outside your doocroot.

If you have svn, you can get the same by doing svn checkout or export from:

http://svn.apache.org/repos/asf/lucene/solr/trunk/contrib/extraction/lib/

