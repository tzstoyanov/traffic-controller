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







