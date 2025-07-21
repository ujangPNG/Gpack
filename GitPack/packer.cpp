#include "packer.h"
#include <git2.h>
#include <iostream>

int create_packfile(const std::string& folderPath, const std::string& outputFile) {
    git_libgit2_init();

    git_repository* repo = nullptr;
    if (git_repository_init(&repo, folderPath.c_str(), false) != 0) {
        std::cerr << "Failed to init repo in folder: " << folderPath << "\n";
        return -1;
    }

    // Tambahkan semua file ke index
    git_index* index;
    git_repository_index(&index, repo);
    git_index_add_all(index, NULL, GIT_INDEX_ADD_DEFAULT, NULL, NULL);
    git_index_write(index);

    // Commit dummy
    git_oid tree_oid, commit_oid;
    git_index_write_tree(&tree_oid, index);

    git_tree* tree;
    git_tree_lookup(&tree, repo, &tree_oid);

    git_signature* sig;
    git_signature_now(&sig, "GitPacker", "gitpacker@example.com");

    if (git_commit_create_v(&commit_oid, repo, "HEAD", sig, sig, NULL, "Pack commit", tree, 0) != 0) {
        std::cerr << "Failed to create commit\n";
        return -1;
    }

    // Generate packfile
    git_odb* odb;
    git_repository_odb(&odb, repo);
    git_odb_pack* pack;
    git_odb_packwriter_new(&pack, odb);
    // ⚠️: Ini perlu diganti dengan API `git_packbuilder` untuk file .pack

    git_libgit2_shutdown();
    return 0;
}
