31 gennaio 2017
  
- da aggiungere i fori non metallizzati



15 aprile 2016

- nello schematico le net Vcc e DGND sono create dalla 3V3 e dalla GND tramite 2 ferriti, ma sono connesse male al uC perch� sull'analogico dovrebbero esserci solo una VDDA e una VSSA "pulite". 
- dall'errore precedente si � generato un problema sul PCB e sul funzionamento della scheda perch� la massa a cui � connesso il uC passa solo dalla ferrite e in caso di alti assorbimenti/rumore non � stabile e resetta il uC. Per risolvere questa situazione occorre fare un corto con le masse utilizzando le vie scoperte.






28 settembre 2015

Problemi riscontrati sulla BAT:

- Il pin 99 del uC (PDR_ON) va scollegato dalla VDD perch� nella revisione Z del uC questo pin � collegato internamente a GND e crea un corto. Si veda l'Errata datasheet.
PDR_ON pin not available on LQFP100 package for revision Z devices

- I 4 fori di fissaggio sono metallizzati a causa di un errore nei gerber e mandano in corto diverse net dei 6 layer presenti. Vanno cambiati i gerber. Workaround: passare una lima nei 4 fori e rimuovere bene la metallizzazione. 

- 4 condensatori non sono stati montati erroneamente, aggiornare schematico e BOM su wingst. (C1, C4, C28, C29)

- La serigrafia vicino al logo IIT sul lato bottom � specchiata

- I pulsanti sono troppo duri, cambiarli con quelli a minor forza

- I pulsanti sono stati montati ruotati di 180gradi, cos� facendo riferimento allo schematico e al nome delle net, il led verde si accende al posto del led rosso e viceversa. La serigrafia dei pulsanti non indica il corretto montaggio, bisogna utilizzare la serigrafia della BCB che � corretta e indica il verso di montaggio.

 