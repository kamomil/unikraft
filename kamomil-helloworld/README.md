"Hello World"-Example for Unikraft
==================================

How I added patches from the mailing list:

1. I set the mutt according to the linux newbies tutorial
In order to get gmail emails I should also set ~/.muttrc:
```
set sendmail="/usr/bin/esmtp"
set envelope_from=yes
set from="Dafna Hirschfeld <dafna3@gmail.com>"
set use_from=yes
set edit_headers=yes

set smtp_url = "smtps://dafna3@gmail.com@smtp.gmail.com:465/"
set smtp_pass = "<gmail pass>"
set imap_user = "dafna3@gmail.com"
set imap_pass = "<gmail pass>"
set folder = "imaps://imap.gmail.com:993"
set spoolfile = "+INBOX"
set ssl_force_tls = yes

```
2. inside mutt, 'c' key asks to switch folder. Then typing '?' gives the list of folders.
Then chhosing 'unikraft' (this is a label in my gmail for unikraft mailing list).
When choosing 'unikraft' it should be with pressing `space` and not `enter`.
3. I found this http://flavioleitner.blogspot.co.il/2011/03/patch-workflow-with-mutt-and-git.html
It worked for me. it needs `sudo apt install procmail` and to create the directories `~/tmp` , `~/incoming`.
also, make sure ~/bin is in $PATH and that `~/bin/from-mutt` is executable (`chmod u+x ~/bin/from-mutt`).
Then as instructed there I type `t` on the mails with the patches I want and then `ctrl+h`
Then the list of patches is saved in `~/incoming`, then in my repo path I can do something like:
`for f in $(find ~/incoming/ | sort); do git am $f; done`






```

MAR
1
Patch workflow with mutt and git
It's easy to grab a patch from a mailing list with mutt and get it merged on your local git tree.
The steps are quite simple:

1. Save a copy of the e-mail
mutt: shift+c (to copy), it will ask:
Copy to mailbox ('?' for list): =mailboxname
then you just need to provide the path, for example: /tmp/testing.mbox

2. Go to your git tree and merge the patch:
cd /path/to/your/git
git am /tmp/testing.mbox

3. If there is no conflict, you are good! Now you can build everything.

However, if you deal with more patches, then the steps above become annoying. Also, usually people have the e-mail on one system and the git tree one another build system. So, I tried to make life easier using a mutt's macro and a script.

The work flow is: You need to tag every patch you like to apply using 't'. Then you run the new macro which in my case is 'ctrl+h' and all patches show up in the build system.

This is the script:

 $ cat ~/bin/from-mutt

----8<-----
#!/bin/bash

PATCH_FILE=`mktemp --tmpdir="$HOME/tmp"  mutt-patch.XXXXXX`
cat > $PATCH_FILE
MAILBOX=`cat $PATCH_FILE | formail -c -xSubject: | tr "'" "." | sed -e '{ s@\[@@g; s@\]@@g; s@[*()" \t]@_@g; s@[/:]@-@g; s@^ \+@@; s@\.\.@.@g; s@-_@_@g; s@__@_@g; s@\.$@@; }' | cut -c 1-70`.patch

mv "$PATCH_FILE" "$HOME/incoming/$MAILBOX"
----8<-----

Note that it uses $HOME/tmp for temporary files and move patches to $HOME/incoming where it can be a local directory or a NFS, for instance.

The sed command is cleaning up the email's subject line to be a filename which resembles the original subject.  You can change it to get better names.

Now that the script is ready (and you have ~/bin in your $PATH), let's create the mutt's macro to use it.  Just copy the line below to your ~/.muttrc:

macro index \Ch "<enter-command>unset wait_key<enter><enter-command>set pipe_decode<enter><tag-prefix><pipe-entry>from-mutt<enter><enter-command>unset pipe_decode<enter><enter-command>set wait_key<enter>" "output git patches"

You also need to add this line to the muttrc:
set pipe_split=yes

Testing:
Here is one example of grabbing patches from netdev mailing:
[...]
Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 2/8] sfc: Reduce size of efx_rx_buffer further by removin
Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 3/8] sfc: Read MC firmware version when requested through
Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 4/8] sfc: Do not read STAT1.FAULT in efx_mdio_check_mmd()
Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 5/8] sfc: Update copyright dates
Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 6/8] sfc: Expose TX push and TSO counters through ethtool
Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 7/8] sfc: Remove configurable FIFO thresholds for pause f
Mar 01 Ben Hutchings [ ] └─>[PATCH net-next-2.6 8/8] sfc: Bump version to 3.1
[...]

Tagging using 't', notice the '*' mark below:
[...]
* Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 2/8] sfc: Reduce size of efx_rx_buffer further by removin
* Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 3/8] sfc: Read MC firmware version when requested through
* Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 4/8] sfc: Do not read STAT1.FAULT in efx_mdio_check_mmd()
* Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 5/8] sfc: Update copyright dates
* Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 6/8] sfc: Expose TX push and TSO counters through ethtool
* Mar 01 Ben Hutchings [ ] ├─>[PATCH net-next-2.6 7/8] sfc: Remove configurable FIFO thresholds for pause f
* Mar 01 Ben Hutchings [ ] └─>[PATCH net-next-2.6 8/8] sfc: Bump version to 3.1
[...]

then after ctrl+h, this shows up in the ~/incoming directory:
$ ls ~/incoming/
_PATCH_net-next-2.6_sfc_Reduce_size_of_efx_rx_buffer_by_unionising_skb_and_page.patch
_PATCH_net-next-2.6_2-8_sfc_Reduce_size_of_efx_rx_buffer_further_by_removing_data_member.patch
_PATCH_net-next-2.6_3-8_sfc_Read_MC_firmware_version_when_requested_through_ethtool.patch
_PATCH_net-next-2.6_4-8_sfc_Do_not_read_STAT1.FAULT_in_efx_mdio_check_mmd_.patch
_PATCH_net-next-2.6_5-8_sfc_Update_copyright_dates.patch
_PATCH_net-next-2.6_6-8_sfc_Expose_TX_push_and_TSO_counters_through_ethtool_statistics.patch
_PATCH_net-next-2.6_7-8_sfc_Remove_configurable_FIFO_thresholds_for_pause_frame_generation.patch
_PATCH_net-next-2.6_8-8_sfc_Bump_version_to_3.1.patch

Now applying the patches:
$ ssh buildsystem
$ cd /storage/net-next-2.6/
$ git am /storage/incoming/*.patch
Applying: sfc: Reduce size of efx_rx_buffer further by removing data member
Applying: sfc: Read MC firmware version when requested through ethtool
Applying: sfc: Do not read STAT1.FAULT in efx_mdio_check_mmd()
Applying: sfc: Update copyright dates
Applying: sfc: Expose TX push and TSO counters through ethtool statistics
Applying: sfc: Remove configurable FIFO thresholds for pause frame generation
Applying: sfc: Bump version to 3.1
$

That's it! Notice that usually the patch ordering is fine. But sometimes, you will need to pass the patches in the right order. In my example, the first patch didn't have the 1/8, so I had to apply it first and then the rest.

You can also tag all patches and then copy using 'shift+c' to a mailbox and try to apply on git as well. But then you won't be able to change the patch order if you need to.
```
