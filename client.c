#include "include/pse.h"
#include "include/erreur.h"
#include "include/ligne.h"
#include "include/resolv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define CMD "client"

void menuCaissier(int sock);
void menuStock(int sock);
void menuManager(int sock);

int main(int argc, char *argv[]) {
    int sock, ret;
    struct sockaddr_in *adrServ;
    int fin = 0;
    char choix[2];
    char identifiant[20];
    char motDePasse[20];

    if (argc != 3)
        erreur("usage: %s machine port\n", argv[0]);

    printf("%s: creating a socket\n", CMD);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        erreur_IO("socket");

    printf("%s: DNS resolving for %s, port %s\n", CMD, argv[1], argv[2]);
    adrServ = resolv(argv[1], argv[2]);
    if (adrServ == NULL)
        erreur("adresse %s port %s inconnus\n", argv[1], argv[2]);

    printf("%s: adr %s, port %hu\n", CMD, stringIP(ntohl(adrServ->sin_addr.s_addr)), ntohs(adrServ->sin_port));

    printf("%s: connecting the socket\n", CMD);
    ret = connect(sock, (struct sockaddr *)adrServ, sizeof(struct sockaddr_in));
    if (ret < 0)
        erreur_IO("connect");

    while (!fin) {
        printf("\n\nMenu Principal :\n");
        printf("1) Caissier\n");
        printf("2) Gestionnaire de stock\n");
        printf("3) Manager\n");
        printf("4) Déconnexion\n");
        printf("Choix : ");
        scanf("%1s", choix);
        ecrireLigne(sock, choix);

        if (choix[0] == '1') {
            printf("Identifiant : ");
            scanf("%s", identifiant);
            ecrireLigne(sock, identifiant);
            printf("Mot de passe : ");
            scanf("%s", motDePasse);
            ecrireLigne(sock, motDePasse);
            menuCaissier(sock);
        } else if (choix[0] == '2') {
            printf("Identifiant : ");
            scanf("%s", identifiant);
            ecrireLigne(sock, identifiant);
            printf("Mot de passe : ");
            scanf("%s", motDePasse);
            ecrireLigne(sock, motDePasse);
            menuStock(sock);
        } else if (choix[0] == '3') {
            printf("Identifiant : ");
            scanf("%s", identifiant);
            ecrireLigne(sock, identifiant);
            printf("Mot de passe : ");
            scanf("%s", motDePasse);
            ecrireLigne(sock, motDePasse);
            menuManager(sock);
        } else if (choix[0] == '4') {
            fin = 1;
        } else {
            printf("Choix invalide, veuillez réessayer.\n");
        }
    }

    if (close(sock) == -1)
        erreur_IO("close socket");

    exit(EXIT_SUCCESS);
}

void menuCaissier(int sock) {
    char choix[2];
    char prenom[20];
    char nom[20];
    char points_fidelite[20];
    char article[30];
    char nbArticles[20];

    while (1) {
        printf("\nMenu Caissier :\n");
        printf("1) Ajouter un client\n");
        printf("2) Enregistrer une vente\n");
        printf("3) Revenir au menu principal\n");
        printf("Choix : ");
        scanf("%1s", choix);
        ecrireLigne(sock, choix);

        if (choix[0] == '1') {
            printf("Prénom : ");
            scanf("%s", prenom);
            ecrireLigne(sock, prenom);
            printf("Nom : ");
            scanf("%s", nom);
            ecrireLigne(sock, nom);
            printf("Points de fidélité : ");
            scanf("%s", points_fidelite);
            ecrireLigne(sock, points_fidelite);
        } else if (choix[0] == '2') {
            printf("Prénom client : ");
            scanf("%s", prenom);
            ecrireLigne(sock, prenom);
            printf("Nom client : ");
            scanf("%s", nom);
            ecrireLigne(sock, nom);
            printf("Article : ");
            scanf("%s", article);
            ecrireLigne(sock, article);
            printf("Nombre d'articles : ");
            scanf("%s", nbArticles);
            ecrireLigne(sock, nbArticles);
        } else if (choix[0] == '3') {
            break;
        } else {
            printf("Choix invalide, veuillez réessayer.\n");
        }
    }
}

void menuStock(int sock) {
    char choix[2];
    char article[30];
    char stock[20];
    char prix[20];

    while (1) {
        printf("\nMenu Gestionnaire de stock :\n");
        printf("1) Modifier le stock\n");
        printf("2) Enregistrer un nouvel article\n");
        printf("3) Revenir au menu principal\n");
        printf("Choix : ");
        scanf("%1s", choix);
        ecrireLigne(sock, choix);

        if (choix[0] == '1') {
            printf("Article : ");
            scanf("%s", article);
            ecrireLigne(sock, article);
            printf("Stock : ");
            scanf("%s", stock);
            ecrireLigne(sock, stock);
        } else if (choix[0] == '2') {
            printf("Article : ");
            scanf("%s", article);
            ecrireLigne(sock, article);
            printf("Stock initial : ");
            scanf("%s", stock);
            ecrireLigne(sock, stock);
            printf("Prix : ");
            scanf("%s", prix);
            ecrireLigne(sock, prix);
        } else if (choix[0] == '3') {
            break;
        } else {
            printf("Choix invalide, veuillez réessayer.\n");
        }
    }
}

void menuManager(int sock) {
    char choix[2];
    char nom[20];
    char prenom[20];
    char fonction[30];
    char date_emploi[20];

    while (1) {
        printf("\nMenu Manager :\n");
        printf("1) Ajouter un employé\n");
        printf("2) Revenir au menu principal\n");
        printf("Choix : ");
        scanf("%1s", choix);
        ecrireLigne(sock, choix);

        if (choix[0] == '1') {
            printf("Prenom : ");
            scanf("%s", prenom);
            ecrireLigne(sock, prenom);
            printf("Nom : ");
            scanf("%s", nom);
            ecrireLigne(sock, nom);
            printf("Fonction : ");
            scanf("%s", fonction);
            ecrireLigne(sock, fonction);
            printf("Date d'emploi : ");
            scanf("%s", date_emploi);
            ecrireLigne(sock, date_emploi);
        } else if (choix[0] == '2') {
            break;
        } else {
            printf("Choix invalide, veuillez réessayer.\n");
        }
    }
}
