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
