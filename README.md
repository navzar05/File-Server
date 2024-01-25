-----------------------------------File Server-----------------------------------

Pentru a testa server-ul, am creat cateva functii care pot fi folosite in
fisierul "client.c".

Rularea proiectului se face dupa cum urmeaza:
	- make clean
	- make
	- intr-un terminal se ruleaza server-ul: "./bin/server"
	- in alt/e terminal/e se ruleaza cilentul/clientii: "./bin/client"


In "main.c" se afla programul principal al server-ului. In "server.c" sunt
implementate functiile pe care server-ul le foloseste.

La nivelul proiectului exista un folder care reprezinta root-ul fisierelor
pe care serverul le pune la dispozitie. Calea catre el este definita in
"server.h". Calea catre fisierul de log, numarul maxim de thread-uri si alte
caracteristici specifice server-ului sunt definite in header.

Pentru caracteristici comune intre server si client, avem macro-uri definite
in "utils.h" (coduri de eroare, codificare comenzilor, IP server, etc.)

Pentru gestionarea "gracious exit", server-ul lanseaza un thread separat
denumit "thread_quit" (din functia "server_init()) care "asteapta" pentru
informatiile specificate in cerinta. In momentul in care se identifica vreunul
dintre semnale sau se tasteaza "quit" la stdin, thread-ul apeleaza
"gracious_exit" care se ocupa de gestionarea "gratioasa" a inchiderii aplicatiei.

Pentru indexarea fisierelor, server-ul porneste un alt thread (tot in functia
server_init()) care creeaza pentru fiecare fisier, folosind structurile definite
in header, o lista cu top 10 cele mai frecvente cuvinte. La apelarea functiei
search(), cautarea se va face in aceasta structura.

Logger-ul este reprezentat de functia log_operation() care apeleaza flock(2)
pe fisierul de log. Aceasta este apelata de fiecare data cand o operatie a
fost efectuata cu succes.

Alte detalii:
	- aplicatia afiseaza toate fisiere din cadrul diretorului root;
	- in cazul descarcarii, daca exista un fisier cu numele fisierului
	  care urmeaza sa fie descarcat, clientul creeaza un nume unic;
	- pentru debug, la descarcare, se afiseaza continutul la stdout;
	- pentru alinierea cu cerintele temei, am creat o noua lista
	  pentru numele fisierelor disponibile pe server, chiar daca aceasta
	  lista se afla si in structura care gestioneaza cele top 10 cele
	  mai frecvente cuvinte din fiecare fisier.

Functiile declarate in "utils.h" si implementate in "utils.c" au fost inspirate de
pe stackoverflow.

Surse

Functiile de trimitere si repceptionare sunt inspirate de aici:
        https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c

        Functiile lsrec sunt inspirate de aici:
        https://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux