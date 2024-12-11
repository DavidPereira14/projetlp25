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

  char line[1024];

  while (fgets(line, sizeof(line), file)) {
    log_element *nv_elmt = malloc(sizeof(log_element));
    if (!nv_elmt) {
      perror("Erreur d'allocation mémoire");
      fclose(file);
      exit(EXIT_FAILURE);
    }

    // Variables temporaires pour stocker les données de la ligne
    char path[256], date[32], md5_str[MD5_DIGEST_LENGTH * 2 + 1];


}

// Fonction permettant de mettre à jour une ligne du fichier .backup_log
void update_backup_log(const char *logfile, log_t *logs){
  /* Implémenter la logique de modification d'une ligne du fichier ".bakcup_log"
  * @param: logfile - le chemin vers le fichier .backup_log
  *         logs - qui est la liste de toutes les lignes du fichier .backup_log sauvegardée dans une structure log_t
  */

}

void write_log_element(log_element *elt, FILE *logfile){
  /* Implémenter la logique pour écrire un élément log de la liste chaînée log_element dans le fichier .backup_log
   * @param: elt - un élément log à écrire sur une ligne
   *         logfile - le chemin du fichier .backup_log
   */
  FILE *f = fopen(logfile, "w");
  if (f == NULL){
    perror("Erreur lors de l'ouverture du fichier");
    }

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