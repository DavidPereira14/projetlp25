#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include "file_handler.h"
#include "deduplication.h"


// Fonction permettant de lire un élément du fichier .backup_log
log_t read_backup_log(const char *logfile){
    /* Implémenter la logique pour la lecture d'une ligne du fichier ".backup_log"
    * @param: logfile - le chemin vers le fichier .backup_log
    * @return: une structure log_t
    */
    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier .backup_log");
        exit(EXIT_FAILURE); // Quitte en cas d'erreur d'ouverture
    }

    // Initialise une liste vide pour stocker les logs
    log_t log_list = { NULL, NULL };

    char line[1024];  // Tableau pour stocker chaque ligne du fichier

    while (fgets(line, sizeof(line), file)) {
        // Crée un nouvel élément pour la liste chaînée
        log_element *new_element = malloc(sizeof(log_element));
        if (!new_element) {
            perror("Erreur d'allocation mémoire");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Parse la ligne en utilisant strtok pour extraire les données
        char *path = strtok(line, ";");
        char *time = strtok(NULL, ";");
        char *md5_str = strtok(NULL, "\n");  // "\n" pour ignorer la fin de ligne

        if (!path || !time || !md5_str) {
            perror("Erreur lors de la récupération des données");
            free(new_element); // Libère l'élément en cas d'erreur
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Remplir l'élément avec les valeurs extraites
        new_element->path = strdup(path);  // Duplication pour éviter les problèmes de gestion de mémoire
        new_element->date = strdup(time);

        // Convertir la chaîne MD5 en tableau d'octets
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            sscanf(md5_str + (i * 2), "%2hhx", &new_element->md5[i]);
        }

        // Ajout de l'élément à la liste chaînée
        new_element->next = NULL;
        new_element->prev = log_list.tail;

        if (log_list.tail) {
            log_list.tail->next = new_element;  // Relie le précédent élément
        }
        log_list.tail = new_element;  // Le nouvel élément devient le dernier

        if (log_list.head == NULL) {
            log_list.head = new_element;  // Si la liste était vide, le nouvel élément est aussi le premier
        }
    }

    fclose(file);
    return log_list;  // Retourne la liste chaînée remplie
}

// Fonction permettant de mettre à jour une ligne du fichier .backup_log
void update_backup_log(const char *logfile, log_t *logs) {
    /* Implémenter la logique de modification d'une ligne du fichier ".backup_log"
     * @param: logfile - le chemin vers le fichier .backup_log
     *         logs - la liste de toutes les lignes du fichier .backup_log sauvegardée dans une structure log_t
     */

    // Ouvre le fichier .backup_log en mode lecture
    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    // Stocke toutes les lignes valides
    char *lines[1024];  // Tableau pour stocker les lignes du fichier
    int line_count = 0;
    char line[1024];     // Buffer pour lire chaque ligne du fichier
    long position;       // Pour stocker la position de chaque ligne

    // Parcours du fichier et stockage des lignes valides
    while (fgets(line, sizeof(line), file)) {
        position = ftell(file);  // Position actuelle dans le fichier

        // Extraire le chemin du fichier de chaque ligne
        char *path_in_file = strtok(line, ";");

        if (path_in_file) {
            // Vérifie si le fichier existe encore
            if (access(path_in_file, F_OK) == -1) {
                // Si le fichier n'existe plus, on ne conserve pas cette ligne
                continue;
            }
        }

        // Si le fichier existe, on conserve la ligne
        lines[line_count] = strdup(line);
        if (!lines[line_count]) {
            perror("Erreur d'allocation mémoire");
            fclose(file);
            return;
        }
        line_count++;
    }

    fclose(file);  // Ferme le fichier après la lecture

    // Ouvre à nouveau le fichier en mode écriture pour le réécrire sans les lignes obsolètes
    file = fopen(logfile, "w");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier en écriture");
        return;
    }

    // Réécriture du fichier avec les lignes valides
    for (int i = 0; i < line_count; i++) {
        fprintf(file, "%s", lines[i]);
        free(lines[i]);  // Libère la mémoire allouée pour la ligne
    }

    fclose(file);  // Ferme le fichier après la réécriture
}

void write_log_element(log_element *elt, FILE *logfile){
  /* Implémenter la logique pour écrire un élément log de la liste chaînée log_element dans le fichier .backup_log
   * @param: elt - un élément log à écrire sur une ligne
   *         logfile - le chemin du fichier .backup_log
   */
  FILE *f = fopen(logfile, "a");
  if (f == NULL){
    perror("Erreur lors de l'ouverture du fichier");
    }
    fseek(logfile, 0, SEEK_END);
    fprintf(f, "%s;%s;%s", elt->path, elt->date, elt->md5);
    fclose(f);
}

void list_files(const char *path){
  /* Implémenter la logique pour lister les fichiers présents dans un répertoire
  */
  DIR *dir = opendir(path);
  if (!dir) {
    perror("Erreur lors de l'ouverture du répertoire");
    return;
  }

  struct dirent *entry;

  // Parcourt les entrées du répertoire
  while ((entry = readdir(dir)) != NULL) {
    printf("%s\n", entry->d_name);
  }

  // Ferme le répertoire
  if (closedir(dir) != 0) {
    perror("Erreur lors de la fermeture du répertoire");
  }
}