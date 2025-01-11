#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include "file_handler.h"
#include "deduplication.h"
#include "backup_manager.h"
#include "network.h"
#include <stdbool.h>



bool verbose = false;

void print_usage(const char *prog_name) {
    printf("Utilisation : %s [OPTIONS]\n", prog_name);
    printf("Options :\n");
    printf("  --backup                : Crée une nouvelle sauvegarde\n");
    printf("  --restore               : Restaure une sauvegarde\n");
    printf("  --list-backups          : Liste les sauvegardes existantes\n");
    printf("  --dry-run               : Effectue une simulation\n");
    printf("  --d-server <IP>         : Adresse IP du serveur de destination\n");
    printf("  --d-port <PORT>         : Port du serveur de destination\n");
    printf("  --s-server <IP>         : Adresse IP du serveur source\n");
    printf("  --s-port <PORT>         : Port du serveur source\n");
    printf("  --dest <CHEMIN>         : Chemin de destination\n");
    printf("  --source <CHEMIN>       : Chemin source\n");
    printf("  -v, --verbose           : Active un affichage détaillé\n");
}

void handle_backup(const char *source, const char *dest, bool dry_run, bool verbose) {
    struct timeval start, end;
    if (verbose) printf("Debut de la sauvegarde de '%s' vers '%s' .\n", source, dest);
    if (dry_run) printf("Simulation activée.\n");
    if (verbose) gettimeofday(&start, NULL);

    create_backup(source, dest);

    if (verbose) {
        gettimeofday(&end, NULL);
        double total_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        printf("Temps d'exécution : %f secondes\n", total_time);
    }
}

void handle_restore(const char *source, const char *dest, bool dry_run, bool verbose) {
    struct timeval start, end;
    if (verbose) printf("Restauration en cours depuis '%s' vers '%s' .\n", source, dest);
    if (dry_run) printf("Simulation activée.\n");
    if (verbose) gettimeofday(&start, NULL);

    restore_backup(source, dest);

    if (verbose) {
        gettimeofday(&end, NULL);
        double total_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        printf("Temps d'exécution : %f secondes\n", total_time);
    }
}

void handle_list_backups(const char *source) {
    printf("Liste des sauvegardes...\n");
    list_backups(source);
}

int main(int argc, char *argv[]) {
    bool backup = false, restore = false, liste_backups = false, dry_run = false, verbose = false;
    const char *d_server = NULL, *s_server = NULL;
    const char *dest = NULL, *source = NULL;
    int d_port = 0, s_port = 0;

    struct option long_options[] = {
            {"backup", no_argument, NULL, 'b'},
            {"restore", no_argument, NULL, 'r'},
            {"list-backups", no_argument, NULL, 'l'},
            {"dry-run", no_argument, NULL, 'd'},
            {"d-server", required_argument, NULL, 'D'},
            {"d-port", required_argument, NULL, 'P'},
            {"s-server", required_argument, NULL, 'S'},
            {"s-port", required_argument, NULL, 'p'},
            {"dest", required_argument, NULL, 't'},
            {"source", required_argument, NULL, 's'},
            {"verbose", no_argument, NULL, 'v'},
            {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "brldD:P:S:p:t:s:v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'b': backup = true; break;
            case 'r': restore = true; break;
            case 'l': liste_backups = true; break;
            case 'd': dry_run = true; break;
            case 'D': d_server = optarg; break;
            case 'P': d_port = atoi(optarg); break;
            case 'S': s_server = optarg; break;
            case 'p': s_port = atoi(optarg); break;
            case 't': dest = optarg; break;
            case 's': source = optarg; break;
            case 'v': verbose = true; break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if ((backup + restore + liste_backups) > 1) {
        fprintf(stderr, "Erreur : Vous ne pouvez spécifier qu'une seule action principale (--backup, --restore, ou --list-backups).\n");
        return EXIT_FAILURE;
    }

    if (backup || restore) {
        if (!source || !dest) {
            fprintf(stderr, "Erreur : Les options --source et --dest sont requises pour cette action.\n");
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (backup) {
        struct timeval start, end;
        long seconds, useconds;
        double total_time;
        printf("Exécution d'une sauvegarde...\n");
        if (dry_run) printf("Simulation activée.\n");
        if (verbose){
            gettimeofday(&start, NULL); // Début du chronométrage
        }
        create_backup(source,dest);
        if (verbose){
            gettimeofday(&end, NULL);   // Fin du chronométrage
            // Calcul de la durée
            seconds = end.tv_sec - start.tv_sec;
            useconds = end.tv_usec - start.tv_usec;
            total_time = seconds + useconds / 1e6;
            printf("Temps d'exécution : %f secondes\n", total_time);
        }
    } else if (restore) {
        struct timeval start, end;
        long seconds, useconds;
        double total_time;
        printf("Restauration en cours...\n");
        if (dry_run) printf("Simulation activée.\n");
        if (verbose) printf("Source : %s, Destination : %s\n", source, dest);
        if (verbose){
            gettimeofday(&start, NULL); // Début du chronométrage
        }
        restore_backup(source,dest);  //backup_id : source ; dest : restore_dir
        if (verbose){
            gettimeofday(&end, NULL);   // Fin du chronométrage
            // Calcul de la durée
            seconds = end.tv_sec - start.tv_sec;
            useconds = end.tv_usec - start.tv_usec;
            total_time = seconds + useconds / 1e6;
            printf("Temps d'exécution : %f secondes\n", total_time);
        }
    } else if (liste_backups) {
        printf("Liste des sauvegardes...\n");
        list_backups(source);
    } else {
        fprintf(stderr, "Erreur : Aucune action spécifiée.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

