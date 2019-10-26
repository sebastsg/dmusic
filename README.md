# dmusic
*mostly automatic archiving, mostly manual listening.*

note: some details of the text below may not be valid yet, as this program is not ready for release.

# terms
* **attachments** are files related to albums or groups, such as booklets, covers, logs, photos, etc.
* **groups** can be bands, artists, projects, etc.
* **group activity** means when a group was officially active. it has nothing to do with album releases.
* **person** means an individual. they can be a member of multiple groups.

# what does it do
this program will archive music by different means, and provide an efficient interface to listen to it.
you can upload zip files directly, or the server can download files remotely from other sources if configured.

each album will go through an import process, where you manually verify that the server's analysis is correct.
if not, you can make changes such as managing attachments, renaming tracks, fixing release year, etc.
multiple releases of an album is supported.
albums with multiple discs is also supported.

groups have details such as description, origin country, website, and activity.
groups can also be related to each other. for example, if they consistently work together, or if the style of music is very 
similar. the relation reason can be specified. relations such as a person being part of two groups are automatic.
group members can be added to specify their role, and when they joined or left.
a person can also contain information, meaning you can go to a group and select a member to see that information.

tracks can have tags added to them, such as *instrumental* or *live*.
these tags are suggested in import after analysis.
otherwise, they must be added manually.

# maintenance
## data integrity
the database contains hashes of all the files, and will periodically verify them.

rip log files are used to tell users to look for alternative sources.
they should always be included when importing albums from non-web releases.
if no log is found, the album will be marked worse than a log with errors.

## album release prediction
when it has been a while since a group has had an album imported, as well as that album being released only a few years ago at the time of import, the user making that import will receive a notification.
this also takes group activity into consideration.
if the group is marked as not being active, the notification will not be given.
it's possible to view a full list in the upload page.

# install
clone repo to destination, ```make``` when inside, then run ```dmusic --install```.
this will create the required directories, install the database as configured in **dmusic.conf**, and then seed the tables with the files under **db/seed**.

postgres is required.
see ```install_thirdparty.sh``` for other dependencies.

# niche features
## server cookies
by default, the server only has access to public resources, but when adding attachments for example, it might be useful to give the server your cookies for accessing private resources as well.
this is done by editing the cookies file, which will be directly consumed by curl.
in the future, a better system will replace this.
probably with a bit more focus on security, and allowing for users to add their cookies themselves.
not sure how yet.
