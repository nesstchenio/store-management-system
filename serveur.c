#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <semaphore.h>

#include "include/pse.h"
#include "include/erreur.h"
#include "include/ligne.h"
#include "include/resolv.h"
#include "include/datathread.h"
#define CMD "serveur"
#define NB_WORKERS 10

pthread_mutex_t mutexEmployes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexClients = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexArticles = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexJournal = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCanal[NB_WORKERS];

int mutex = 0;

typedef struct Client {
    char prenom[20];
    char nom[20];
    float points_fidelite;
} Client;

typedef struct Article {
    char nom[30];
    int stock;
    float prix;
} Article;

typedef struct Employe {
	char prenom[20];
	char nom[20];
	char fonction[30];
	char date_emploi[10];
} Employe;

void creerCohorteWorkers(void);
int chercherWorkerLibre(void);
void *threadWorker(void *arg);
void sessionClient(int canal);
void remiseAZeroJournal(void);
int modifier_stock(const char* article_nom, int nb);
Client* maj_client(void);
Article* maj_articles(void);
int ajouter_client(char *prenom, char *nom, float points_fidelite);
int ajouter_article(char *nom, int quantite, float prix);
float stringToFloat(char *str);
int stringToInt(char *str);
int modifier_points_fidelite(const char* prenom,const char* nom, float montant);
Client* trouver_client(const char* prenom, const char* nom);
Article* trouver_article(const char* nom);
int enregistrer_vente(const char* prenom, const char* nom, const char* article_nom, int quantite);
int ajouter_employe(const char* nom, const char* prenom, const char* fonction, const char* date_emploi);


float stringToFloat(char *str) {
/*Transforme une chaîne de caractère représentant un flottant en flottant*/
    float result = 0.0;
    int sign = 1;
    int i = 0;
   
    /*Cas d'un nombre négatif*/
    if (str[0] == '-')
    {
        sign = -1;
        i = 1;
    }
   
    /*Partie entière*/
    while (str[i] != '\0' && str[i] != '.')
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            result = result * 10 + (str[i] - '0');
        }
        else
            return 0.0;
        i++;
    }
   
    /*Partie décimale*/
    if (str[i] == '.')
    {
        float fraction = 1.0;
        i++;
       
        while (str[i] != '\0') {
            if (str[i] >= '0' && str[i] <= '9')
            {
                fraction *= 0.1;
                result = result + (str[i] - '0') * fraction;
            }
            else
                return 0.0;
            i++;
        }
    }
   
    result *= sign;
   
    return result;
}


int stringToInt(char *str)
/*Transforme une chaîne de caractère représentant un entier en entier*/
{
    int result = 0;
    int sign = 1;
    int i = 0;
   
    /*Gérer le signe négatif*/
    if (str[0] == '-')
    {
        sign = -1;
        i = 1;
    }
   
    /*Parcours des numéros de la chaine*/
    while (str[i] != '\0')
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            result = result * 10 + (str[i] - '0');
        }
        else
            return 0;
        i++;
    }
   
    result *= sign;
   
    return result;
}


int fdJournal;
DataSpec dataSpec[NB_WORKERS];

int modifier_points_fidelite(const char* prenom, const char* nom, float total) {
    FILE* file = fopen("Files/clients.txt", "r+");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier clients.txt");
        return -1;
    }

    pthread_mutex_lock(&mutexClients);

    // Lire et ignorer la première ligne
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Erreur de lecture de la première ligne");
        pthread_mutex_unlock(&mutexClients);
        fclose(file);
        return -1;
    }

    // Parcourir le fichier pour trouver le client
    long pos = ftell(file);
    Client temp;
    while (fscanf(file, "%s %s %f", temp.prenom, temp.nom, &temp.points_fidelite) == 3) {
        if (strcmp(temp.prenom, prenom) == 0 && strcmp(temp.nom, nom) == 0) {
            break;
        }
        // Mise à jour de la position du curseur
        pos = ftell(file);
    }

    // Vérification si le client a été trouvé
    if (strcmp(temp.prenom, prenom) != 0 && strcmp(temp.nom, nom) != 0) {
        printf("Client non trouvé\n");
        pthread_mutex_unlock(&mutexClients);
        fclose(file);
        return -1;
    }

    // Modification des points de fidélité du client
    temp.points_fidelite += (total/10);

    // Positionner le curseur de fichier au bon endroit pour écrire le nouveau total de points_fidelite
    fseek(file, pos, SEEK_SET);

    // Écriture du nouveau total de points_fidelité dans le fichier
    fprintf(file, "\n%s %s %.4f\n", temp.prenom, temp.nom, temp.points_fidelite);

    pthread_mutex_unlock(&mutexClients);
    fclose(file);
    return 1;
}

Client* trouver_client(const char* prenom, const char* nom) {
    FILE* file = fopen("Files/clients.txt", "r");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier clients.txt");
        return NULL;
    }

    Client* client = (Client*)malloc(sizeof(Client));
    if (client == NULL) {
        perror("Erreur d'allocation mémoire");
        fclose(file);
        return NULL;
    }

    // Lire et ignorer la première ligne
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Erreur de lecture de la première ligne");
        fclose(file);
        free(client);
        return NULL;
    }

    // Lire les lignes suivantes pour trouver le client
    while (fscanf(file, "%s %s %f", client->prenom, client->nom, &client->points_fidelite) == 3) {
        // Débogage : Afficher ce qui est lu
        printf("Lu: %s %s %f\n", client->prenom, client->nom, client->points_fidelite);
        if (strcmp(client->prenom, prenom) == 0 && strcmp(client->nom, nom) == 0) {
            printf("Le client a été trouvé\n");
            fclose(file);
            return client;
        }
    }
    printf("Le client n'existe pas\n");
    fclose(file);
    free(client);
    return NULL;
}

int main(int argc, char *argv[]) {
    short port;
    int ecoute, canal, ret;
    struct sockaddr_in adrEcoute, adrClient;
    unsigned int lgAdrClient;
    int numWorkerLibre;

    mkdir("Files", 0777);

    creerCohorteWorkers();

    if (argc != 2)
        erreur("usage: %s port\n", argv[0]);

    port = (short)atoi(argv[1]);

    printf("%s: creating a socket\n", CMD);
    ecoute = socket(AF_INET, SOCK_STREAM, 0);
    if (ecoute < 0)
        erreur_IO("socket");

    adrEcoute.sin_family = AF_INET;
    adrEcoute.sin_addr.s_addr = INADDR_ANY;
    adrEcoute.sin_port = htons(port);
    printf("%s: binding to INADDR_ANY address on port %d\n", CMD, port);
    ret = bind(ecoute, (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
    if (ret < 0)
        erreur_IO("bind");

    printf("%s: listening to socket\n", CMD);
    ret = listen(ecoute, 5);
    if (ret < 0)
        erreur_IO("listen");

    while (1) {
        printf("%s: accepting a connection\n", CMD);
        lgAdrClient = sizeof(adrClient);
        canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
        if (canal < 0)
            erreur_IO("accept");

        printf("%s: adr %s, port %hu\n", CMD, stringIP(ntohl(adrClient.sin_addr.s_addr)), ntohs(adrClient.sin_port));

        while ((numWorkerLibre = chercherWorkerLibre()) < 0)
            usleep(100000);

        dataSpec[numWorkerLibre].canal = canal;
    }

    if (close(ecoute) == -1)
        erreur_IO("fermeture ecoute");

    exit(EXIT_SUCCESS);
}


void creerCohorteWorkers(void) {
    int i, ret;

    for (i = 0; i < NB_WORKERS; i++) {
        dataSpec[i].canal = -1;
        dataSpec[i].tid = i;

        ret = pthread_create(&dataSpec[i].id, NULL, threadWorker, &dataSpec[i]);
        if (ret != 0)
            erreur_IO("creation thread");

        pthread_mutex_init(&mutexCanal[i], NULL);
    }
}

int chercherWorkerLibre(void) {
    int i = 0;

    while (dataSpec[i].canal != -1 && i < NB_WORKERS)
        i++;

    if (i < NB_WORKERS)
        return i;
    else
        return -1;
}

void *threadWorker(void *arg) {
    DataSpec *dataTh = (DataSpec *)arg;

    while (1) {
        while (dataTh->canal < 0)
            usleep(100000);
        printf("%s: worker %d reveil\n", CMD, dataTh->tid);

        sessionClient(dataTh->canal);

        dataTh->canal = -1;
        printf("%s: worker %d sommeil\n", CMD, dataTh->tid);
    }

    pthread_exit(NULL);
}

void sessionClient(int canal) {
    int fin = 0;
    char choix[2];
    char prenom[20];
    char nom[20];
    char points_fidelite[20];
    char article[30];
    char stock[20];
    char prix[20];
    char fonction[30];
    char date_emploi[20];
    char nbArticles[20];
    char identifiant[20];
    char motDePasse[20];

while (!fin) {
        lireLigne(canal, choix);
        if (choix[0] == '1') {
            // Identification Caissier
            lireLigne(canal, identifiant);
            lireLigne(canal, motDePasse);
            if (strcmp(identifiant, "caissier") == 0 && strcmp(motDePasse, "caissier") == 0) {
                while (1) {
                    lireLigne(canal, choix);
                    if (choix[0] == '1') {
                        lireLigne(canal, prenom);
                        lireLigne(canal, nom);
                        lireLigne(canal, points_fidelite);
                        ajouter_client(prenom, nom, stringToFloat(points_fidelite));
                    } else if (choix[0] == '2') {
                        lireLigne(canal, prenom);
                        lireLigne(canal, nom);
                        lireLigne(canal, article);
                        lireLigne(canal, nbArticles);
                        enregistrer_vente(prenom, nom, article, stringToInt(nbArticles));
                    } else if (choix[0] == '3')  {
                    fin = 1;
                    }
                }
            } else {
                printf("l'identification a echoue, revenez au menu prinipal");
                break;
            }
            } 
        else if (choix[0] == '2') {
            // Identification Gestionnaire de stock
            lireLigne(canal, identifiant);
            lireLigne(canal, motDePasse);
            if (strcmp(identifiant, "stock") == 0 && strcmp(motDePasse, "stock") == 0) {
                while (1) {
                    lireLigne(canal, choix);
                    if (choix[0] == '1') {
                        lireLigne(canal, article);
                        lireLigne(canal, stock);
                        modifier_stock(article, stringToInt(stock));
                    } else if (choix[0] == '2') {
                        lireLigne(canal, article);
                        lireLigne(canal, stock);
                        lireLigne(canal, prix);
                        ajouter_article(article, stringToInt(stock), stringToFloat(prix));
                    } else if (choix[0] == '3') {
                        fin = 1;
                    }
                }
            } else {
                printf("l'identification a echoue, revenez au menu prinipal");
                fin=1;
                break;
            }
        } else if (choix[0] == '3') {
            // Identification Manager
            lireLigne(canal, identifiant);
            lireLigne(canal, motDePasse);
            if (strcmp(identifiant, "manager") == 0 && strcmp(motDePasse, "manager") == 0) {
                while (1) {
                    lireLigne(canal, choix);
                    if (choix[0] == '1') {
                        lireLigne(canal, prenom);
                        lireLigne(canal, nom);
                        lireLigne(canal, fonction);
                        lireLigne(canal, date_emploi);
                        ajouter_employe(prenom, nom, fonction, date_emploi);
                    } else if (choix[0] == '2') {
                        break;
                    }
                }
            } else {
                printf("l'identification a echoue, revenez au menu prinipal");
                break;
            }
        } else if (choix[0] == '4') {
            fin = 1;
        }
    }
}


int ajouter_client(char *prenom, char *nom, float points_fidelite) {
    int nbClients;
    char buffer[20];
    int i; /* Compteur boucle for */
    Client *repertoire;
    FILE *fd;

    if (pthread_mutex_lock(&mutexClients) != 0) {
        perror("mutex_lock");
        exit(EXIT_FAILURE);
    }
    mutex = 1;

    fd = fopen("Files/clients.txt", "r");
    if (fd == NULL) {
        perror("Erreur 1 d'ouverture du fichier clients.txt");
        pthread_mutex_unlock(&mutexClients);
        return 1;
    }
if (fscanf(fd, "%s %s %s %d", buffer, buffer, buffer, &nbClients) != 4) {
    printf("Erreur lors de la lecture du nombre de clients\n");
    return EXIT_FAILURE;
}
    fscanf(fd, "%s %s %s %d", buffer, buffer, buffer, &nbClients); /* Première lecture pour récupérer le nombre de cotisants */
    printf("%d",nbClients);
    fclose(fd);
    if (pthread_mutex_unlock(&mutexClients) != 0) {
        perror("mutex_unlock");
        exit(EXIT_FAILURE);
    }
    mutex = 0;

    repertoire = (Client *)malloc(sizeof(Client) * nbClients); /* Sauvegarde du contexte des cotisants */
    if (repertoire == NULL) {
        perror("Erreur d'allocation mémoire");
        return -1;
    }

    repertoire = maj_client();
    if (repertoire == NULL) {
        free(repertoire);
        return -1;
    }

    for (i = 0; i < nbClients; i++) {
        if (strcmp(prenom, repertoire[i].prenom) == 0 && strcmp(nom, repertoire[i].nom) == 0) { /* Si le cotisant existe déjà dans la liste */
            printf("\nClient déjà existant\n\n");
            free(repertoire);
            return -1;
        }
    }

    if (pthread_mutex_lock(&mutexClients) != 0) {
        perror("mutex_lock");
        exit(EXIT_FAILURE);
    }
    mutex = 1;

    fd = fopen("Files/clients.txt", "w");
    if (fd == NULL) {
        perror("Erreur d'ouverture du fichier clients.txt");
        pthread_mutex_unlock(&mutexClients);
        free(repertoire);
        return 1;
    }

    fprintf(fd, "Client\tPoints_fidelite\t\tnbClients: %d\n", nbClients + 1); /* Ecrasement des données */
    for (i = 0; i < nbClients; i++) {
        fprintf(fd, "%s %s\t\t%f\n", repertoire[i].prenom, repertoire[i].nom, repertoire[i].points_fidelite);
    }
    fprintf(fd, "%s %s\t\t%f\n", prenom, nom, points_fidelite); /* Ajout du nouveau cotisant en fin de liste */
    fclose(fd);

    if (pthread_mutex_unlock(&mutexClients) != 0) {
        perror("mutex_unlock");
        exit(EXIT_FAILURE);
    }
    mutex = 0;

    free(repertoire);
    return 1;
}

int ajouter_article(char* nom, int stock, float prix) {
    FILE *file;
    int ret;
    int nbArticles = 0;
    char buffer[256];
    long pos;

    ret = pthread_mutex_lock(&mutexArticles);
    if (ret != 0) {
        perror("Erreur lors du verrouillage du mutex");
        return -1;
    }

    file = fopen("Files/articles.txt", "r+");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier articles.txt");
        pthread_mutex_unlock(&mutexArticles);
        return -1;
    }

    // Lire la première ligne pour obtenir nbArticles
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        sscanf(buffer, "Article\tStock\tPrix\tnbArticles: %d", &nbArticles);
    } else {
        perror("Erreur lors de la lecture de la première ligne");
        fclose(file);
        pthread_mutex_unlock(&mutexArticles);
        return -1;
    }

    // Rechercher l'article pour éviter les doublons
    if (trouver_article(nom) != NULL) {
        printf("\nArticle déjà proposé\n\n");
        fclose(file);
        pthread_mutex_unlock(&mutexArticles);
        return -1;
    }

    // Augmenter le nombre d'articles
    nbArticles++;

    // Repositionner le fichier au début pour mettre à jour le compteur
    pos = ftell(file); // Sauvegarder la position actuelle
    fseek(file, 0, SEEK_SET);
    fprintf(file, "Article\tStock\tPrix\tnbArticles: %d\n", nbArticles);
    fseek(file, pos, SEEK_SET); // Revenir à la position sauvegardée

    // Ajouter le nouvel article à la fin du fichier
    fseek(file, 0, SEEK_END);
    fprintf(file, "%s %d %f\n", nom, stock, prix);

    if (fclose(file) != 0) {
        perror("Erreur lors de la fermeture du fichier articles.txt");
        pthread_mutex_unlock(&mutexArticles);
        return -1;
    }

    ret = pthread_mutex_unlock(&mutexArticles);
    if (ret != 0) {
        perror("Erreur lors du déverrouillage du mutex");
        return -1;
    }

    return 0;
}

int enregistrer_vente(const char* prenom, const char* nom, const char* article_nom, int quantite) {
    Client* client;
    Article* article;
    int isStock;
    float total;

    // Recherche du client
    client = trouver_client(prenom, nom);
    if (client == NULL) {
        // Si le client n'existe pas, l'ajouter
        ajouter_client((char *)prenom, (char *)nom, 0);
        client = trouver_client(prenom, nom);  // Rechercher à nouveau le client
    }

    // Recherche de l'article
    article = trouver_article(article_nom);
    if (article == NULL) {
        printf("\nArticle indisponible\n");
        return -1;
    }

    // Calcul du total de la commande
    total = article->prix * quantite;

    // Vérification du stock et mise à jour si suffisant
    isStock = modifier_stock(article->nom, quantite);
    if (isStock > 0) {
        // Mise à jour du points_fidelite du client
        modifier_points_fidelite(client->prenom, client->nom, total);
    } else {
        printf("\nLes stocks ne sont pas suffisants, l'achat n'a pas pu être réalisé\n");
        return -1;
    }
    return 1;
}

int modifier_stock(const char* article_nom, int nb) {
    FILE* file = fopen("Files/articles.txt", "r+");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier articles.txt");
        return -1;
    }

    pthread_mutex_lock(&mutexArticles);

    // Lire et ignorer la première ligne
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Erreur de lecture de la première ligne");
        pthread_mutex_unlock(&mutexArticles);
        fclose(file);
        return -1;
    }

    // Parcourir le fichier pour trouver l'article
    long pos = 0;
    Article temp;
    while (fscanf(file, "%s %d %f", temp.nom, &temp.stock, &temp.prix) == 3) {
        if (strcmp(temp.nom, article_nom) == 0) {
            pos = ftell(file) - (long)(strlen(temp.nom) + 1 + sizeof(temp.stock) + 1 + sizeof(temp.prix) + 1);
            break;
        }
    }

    if (pos == 0) {
        printf("Article non trouvé\n");
        pthread_mutex_unlock(&mutexArticles);
        fclose(file);
        return -1;
    }

    // Modification du stock de l'article
    temp.stock -= nb;
    if (temp.stock < 0) {
        printf("\nErreur au niveau du stock\n");
        pthread_mutex_unlock(&mutexArticles);
        fclose(file);
        return -1;
    }

    // Positionner le curseur de fichier au bon endroit pour écrire le nouveau stock
    fseek(file, pos, SEEK_SET);

    // Écriture du nouveau stock dans le fichier
    fprintf(file, "%s %d %.4f", temp.nom, temp.stock, temp.prix);

    pthread_mutex_unlock(&mutexArticles);

    fclose(file);
    return 1;
}

Client* maj_client(void) {
    int nbClients;
    char buffer[20];
    int i; /* Compteur boucle for */
    Client *res;
    FILE *fd;

    if (pthread_mutex_lock(&mutexClients) != 0) {
        perror("mutex_lock");
        exit(EXIT_FAILURE);
    }
    mutex = 1;

    fd = fopen("Files/clients.txt", "r");
    if (fd == NULL) {
        perror("Erreur d'ouverture du fichier clients.txt");
        pthread_mutex_unlock(&mutexClients);
        return NULL;
    }

    fscanf(fd, "%s %s %s %d", buffer, buffer, buffer, &nbClients); /* Première lecture pour récupérer le nombre de cotisants */
    res = (Client *)malloc(sizeof(Client) * nbClients); /* Tableau avec toutes les infos sur les cotisants */
    if (res == NULL) {
        perror("Erreur d'allocation mémoire");
        fclose(fd);
        pthread_mutex_unlock(&mutexClients);
        return NULL;
    }

    for (i = 0; i < nbClients; i++) {
        fscanf(fd, "%s %s %f", res[i].prenom, res[i].nom, &res[i].points_fidelite);
    }

    fclose(fd);
    if (pthread_mutex_unlock(&mutexClients) != 0) {
        perror("mutex_unlock");
        exit(EXIT_FAILURE);
    }
    mutex = 0;

    return res;
}

Article* maj_articles(void){
    int nbArticles;
    char buffer[20];
    int i; /* Compteur boucle for */
    Article *res;
    FILE *fd;

    if (pthread_mutex_lock(&mutexArticles) != 0) {
        perror("mutex_lock");
        exit(EXIT_FAILURE);
    }
    mutex = 1;

    fd = fopen("Files/articles.txt", "r");
    if (fd == NULL) {
        perror("Erreur d'ouverture du fichier articles.txt");
        pthread_mutex_unlock(&mutexArticles);
        return NULL;
    }

    fscanf(fd, "%s %s %s %s %d", buffer, buffer, buffer, buffer, &nbArticles); /* Première lecture pour récupérer le nombre de cotisants */
    res = (Article *)malloc(sizeof(Article) * nbArticles); /* Tableau avec toutes les infos sur les cotisants */
    if (res == NULL) {
        perror("Erreur d'allocation mémoire");
        fclose(fd);
        pthread_mutex_unlock(&mutexArticles);
        return NULL;
    }

    for (i = 0; i < nbArticles; i++) {
        fscanf(fd, "%s %d %f", res[i].nom, &res[i].stock, &res[i].prix);
    }

    fclose(fd);
    if (pthread_mutex_unlock(&mutexArticles) != 0) {
        perror("mutex_unlock");
        exit(EXIT_FAILURE);
    }
    mutex = 0;

    return res;
}

Article* trouver_article(const char* nom) {
    FILE* file = fopen("Files/articles.txt", "r");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier Files/articles.txt");
        return NULL;
    }

    Article* article = (Article*)malloc(sizeof(Article));
    if (article == NULL) {
        perror("Erreur d'allocation mémoire");
        fclose(file);
        return NULL;
    }

   // Lire et ignorer la première ligne
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Erreur de lecture de la première ligne");
        fclose(file);
        free(article);
        return NULL;
    }

    while (fscanf(file, "%s %d %f", article->nom, &article->stock, &article->prix) ==3) {
   
    // Débogage : Afficher ce qui est lu
        printf("Lu: %s %d %f", article->nom, article->stock, article->prix);
        if (strcmp(article->nom, nom) == 0) {
            printf("\nL'article a été trouvé\n");
            fclose(file);
            return article;
        }
    }
    printf("L'article n'existe pas\n");
    fclose(file);
    free(article);
    return NULL;
}

int ajouter_employe(const char* nom, const char* prenom, const char* fonction, const char* date_emploi) {
    FILE *file;
    int ret;
    int nbEmployes;
    char buffer[256];

    ret = pthread_mutex_lock(&mutexEmployes);
    if (ret != 0) {
        perror("Erreur lors du verrouillage du mutex");
        return -1;
    }

    file = fopen("Files/employes.txt", "r+");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier employes.txt");
        pthread_mutex_unlock(&mutexEmployes);
        return -1;
    }

    // Lire la première ligne pour obtenir nbEmployes
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        sscanf(buffer, "employe fonction date_emploi nbEmployes: %d", &nbEmployes);
    } else {
        perror("Erreur lors de la lecture de la première ligne");
        fclose(file);
        pthread_mutex_unlock(&mutexEmployes);
        return -1;
    }

    // Vérifier si l'employé existe déjà
    char tempNom[50], tempPrenom[50], tempFonction[50], tempDate[50];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        sscanf(buffer, "%s %s %s %s", tempNom, tempPrenom, tempFonction, tempDate);
        if (strcmp(tempNom, nom) == 0 && strcmp(tempPrenom, prenom) == 0) {
            printf("L'employé %s %s existe déjà.\n", prenom, nom);
            fclose(file);
            pthread_mutex_unlock(&mutexEmployes);
            return -1;
        }
    }

    // Revenir au début du fichier pour réécrire la première ligne
    fseek(file, 0, SEEK_SET);
    nbEmployes += 1;
    fprintf(file, "employe fonction date_emploi nbEmployes: %d\n", nbEmployes);

    // Passer à la fin du fichier pour ajouter le nouvel employé
    fseek(file, 0, SEEK_END);
    fprintf(file, "%s %s %s %s\n", nom, prenom, fonction, date_emploi);

    if (fclose(file) != 0) {
        perror("Erreur lors de la fermeture du fichier employes.txt");
        pthread_mutex_unlock(&mutexEmployes);
        return -1;
    }

    ret = pthread_mutex_unlock(&mutexEmployes);
    if (ret != 0) {
        perror("Erreur lors du déverrouillage du mutex");
        return -1;
    }

    return 0;
}
