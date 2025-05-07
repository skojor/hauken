
timeout 30

.\zip -j -m %1.zip %1
.\curl -H 'application/x-www-form-url-encoded' -F "brukernavn=%2" -F "passord=%3" --cookie-jar ~/.kake "http://stoygolvet.npta.no/logon.php"
.\curl --cookie ~/.kake -X POST -F file=@%1.zip http://stoygolvet.npta.no/Casper/lastopp_fil.php
