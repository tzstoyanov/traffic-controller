if you want to upload,
1) put file with _netrc name in home folder
with following content:
machine code.google.com 
login <******>
password <******>

2) remove the user name from the URL in the .git/config file

url = https://<USER>@code.google.com/p/traffic-controller/

becomes

url = https://code.google.com/p/traffic-controller/
---------------------------------------------------------------
commit with sign 


1) setup
gpg --list-secret-keys | grep ^sec

взема се числото след "/"
example: sec   4096R/DC9BA80B 2012-08-09 [expires: 2022-08-07]
>>>DC9BA80B<<<

 git config --global user.signingkey  DC9BA80B

By default (at least in Git v1.7.9), git log will not list or validate signatures. In order to display the signature for our commit, we may use the --show-signature option, as shown below:
git log --show-signature

There is an important distinction to be made here Ч the commit author and the signature attached to the commit may represent two different people. In other words: the commit signature is similar in concept to the -s option, which adds a Signed-off line to the commit Ч it verifies that you have signed off on the commit, but does not necessarily imply that you authored it. Example:

git commit -asm 'Signed off'   





