#include "network.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

void send_data(const char *server_address, int port, const void *data, size_t size) {
    // Implémenter la logique d'envoi de données à un serveur distant
    int sockfd;
    struct sockaddr_in server_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {   // Créer la socket
        perror("Échec de la création de la socket");
        return;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {   // Convertir les adresses IPv4 et IPv6 de texte en binaire
        perror("Adresse invalide / Adresse non supportée");
        close(sockfd);
        return;
    }
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {    // Se connecter au serveur
        perror("Échec de la connexion");
        close(sockfd);
        return;
    }
    ssize_t sent_size = send(sockfd, data, size, 0);    // Envoyer les données
    if (sent_size < 0) {
        perror("Échec de l'envoi");
    } else if (sent_size != size) {
        fprintf(stderr, "Attention : la taille envoyée (%zd) ne correspond pas à la taille des données (%zu)\n", sent_size, size);
    }
    close(sockfd);  // Fermer la socket
}

void receive_data(int port, void **data, size_t *size) {
    // Implémenter la logique de réception de données depuis un serveur distant
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    ssize_t read_size;
    size_t buffer_size = 1024;  // Création d'un tampon pour recevoir des données
    *data = malloc(buffer_size);
    if (*data == NULL) {
        perror("Échec de l'allocation de mémoire");
        return;
    }
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {   // Créer le descripteur de fichier socket
        perror("Échec de la création de la socket");
        free(*data);
        return;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {    // Attacher la socket au port spécifié
        perror("Échec de setsockopt");
        close(server_fd);
        free(*data);
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {    // Lier la socket à l'adresse réseau et au port
        perror("Échec de la liaison");
        close(server_fd);
        free(*data);
        return;
    }
    if (listen(server_fd, 3) < 0) {     // Écouter les connexions entrantes
        perror("Échec de l'écoute");
        close(server_fd);
        free(*data);
        return;
    }
    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {   // Accepter une connexion
        perror("Échec de l'acceptation");
        close(server_fd);
        free(*data);
        return;
    }
    *size = 0;   // Recevoir les données
    while ((read_size = recv(client_fd, *data + *size, buffer_size - *size, 0)) > 0) {
        *size += read_size;
        if (*size == buffer_size) {
            buffer_size *= 2;
            *data = realloc(*data, buffer_size);
            if (*data == NULL) {
                perror("Échec de la réallocation de mémoire");
                close(client_fd);
                close(server_fd);
                return;
            }
        }
    }
    if (read_size < 0) {
        perror("Échec de la réception");
        free(*data);
        *data = NULL;
        *size = 0;
    }
    close(client_fd);   // Fermer les sockets
    close(server_fd);
}

