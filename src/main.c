#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "file_handler.h"
#include "deduplication.h"
#include "backup_manager.h"
#include "network.h"

int main(int argc __attribute__((unused)), char *argv[]) {
    // Analyse des arguments de la ligne de commande
    if (argc<3){
        return EXIT_FAILURE;
    }
    // Implémentation de la logique de sauvegarde et restauration
    // Exemples : gestion des options --backup, --restore, etc.
    create_backup(argv[1],argv[2]);
    return EXIT_SUCCESS;
}

