# Mailbox_server

Drei Haupt-Tasks/Prozesse:
•	LED Blink 
•	UDP-Server
•	HTTPS-Server

## Main

Als erstes wird der LED Blink Task created, damit die Programmzustände von Anfang an kommuniziert werden können.
Anschließend wird versucht eine Verbindung zum Heimnetzwerk per WLAN aufzubauen. Das Programm läuft erst weiter, wenn eine Verbindung geschaffen wurde.
Danach wird der UDP Data Mutex initialisiert und der UDP-Server Task created.
Abschließend wird der HTTPS-Server gestartet. 

## LED Blink

Der LED Blink Task, wie der Name schon verrät, lässt die RGB LED vom DevKitM-1 in den entsprechenden Farben erleuchten:

•	Blinkend rot: 	WLAN nicht verbunden

•	konstant blau:	WLAN verbunden keine Post

•	konstant grün: 	WLAN verbunden und Post vorhanden

## UDP-Server

Nach der Erstellung des Taks erzeugt dieser einen socket und bindet sich an diesen. Anschließen springt er in einen Endlosschleife, in der er auf Daten wartet. 
Sobald er Daten erhalten hat, speichert er diese in den udp_data_t struct udp_data (mit Semaphoren abgesichert) und Schickt dem Sender eine entsprechenden Antwort.
Danach wird wieder auf neue Daten gewartet.

## HTTPS-Server

Der HTTPS Server ist Event-gesteuert. Es sind zwei URI-Handler registriert:

•	Root Handler:
Wenn nur die IP-Adresse vom Server aufgerufen wird, stellt der root handler die HTML-Page zur Verfügung.

•	Data Handler:
  Der Data Handler wird regelmäßig von der HTML-Page selbst aufgerufen. Der Data Handler liest die Daten aus dem struct udp_data mit der Funktion get_udp_data() aus. Diese Funktion verwendet beim Zugriff auf udp_data Semaphoren. Anschließend schickt der der 
  handler die Daten in einem JSON-Format an die HTLM-Page.
  In der HTML-Page selbst ist wie schon erwähnt JavaScript Code implementiert, der den Data Handler regelmäßig aufruft, um die angezeigten Daten upzudaten.
  Da es ein HTTPS Server ist benötigt er zusätzlich ein certificate und ein private key. Diese self signed certificates wurden mit Hilfe des OpenSSL Programms erstellt. 
  
Folgende Befehle wurden verwendet:

•	Generate a private key
  openssl genpkey -algorithm RSA -out private_key.pem -aes256
  
•	Generate a CSR
  openssl req -new -key private_key.pem -out csr.pem 
  
•	Generate a self-signed certificate
  openssl req -x509 -key private_key.pem -in csr.pem-out certificate.pem-days 365 
