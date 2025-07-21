#include "packer.h"
#include <git2.h>
#include <iostream>
#include <filesystem>
#include <fstream>

int create_packfile(const std::string& folderPath, const std::string& outputFile) {
    git_libgit2_init();

    git_repository* repo = nullptr;
    git_packbuilder* pb = nullptr;
    int error = 0;

    // Initialize repository
    if (git_repository_init(&repo, folderPath.c_str(), false) != 0) {
        std::cerr << "Failed to init repo in folder: " << folderPath << "\n";
        const git_error* e = git_error_last();
        if (e) std::cerr << "Error: " << e->message << "\n";
        git_libgit2_shutdown();
        return -1;
    }

    // Get repository index
    git_index* index;
    if ((error = git_repository_index(&index, repo)) != 0) {
        std::cerr << "Failed to get repository index\n";
        goto cleanup;
    }

    // Add all files to index
    if ((error = git_index_add_all(index, NULL, GIT_INDEX_ADD_DEFAULT, NULL, NULL)) != 0) {
        std::cerr << "Failed to add files to index\n";
        goto cleanup;
    }

    // Write index
    if ((error = git_index_write(index)) != 0) {
        std::cerr << "Failed to write index\n";
        goto cleanup;
    }

    // Create tree from index
    git_oid tree_oid, commit_oid;
    if ((error = git_index_write_tree(&tree_oid, index)) != 0) {
        std::cerr << "Failed to write tree\n";
        goto cleanup;
    }

    // Create commit
    git_tree* tree;
    if ((error = git_tree_lookup(&tree, repo, &tree_oid)) != 0) {
        std::cerr << "Failed to lookup tree\n";
        goto cleanup;
    }

    git_signature* sig;
    if ((error = git_signature_now(&sig, "GitPacker", "gitpacker@example.com")) != 0) {
        std::cerr << "Failed to create signature\n";
        goto cleanup;
    }

    if ((error = git_commit_create_v(&commit_oid, repo, "HEAD", sig, sig, NULL, "Pack commit", tree, 0)) != 0) {
        std::cerr << "Failed to create commit\n";
        git_signature_free(sig);
        git_tree_free(tree);
        goto cleanup;
    }

    // Create packbuilder
    if ((error = git_packbuilder_new(&pb, repo)) != 0) {
        std::cerr << "Failed to create packbuilder\n";
        git_signature_free(sig);
        git_tree_free(tree);
        goto cleanup;
    }

    // Insert commit into pack
    if ((error = git_packbuilder_insert_commit(pb, &commit_oid)) != 0) {
        std::cerr << "Failed to insert commit into pack\n";
        goto cleanup_all;
    }

    // Create output directory if it doesn't exist
    {
        std::filesystem::path outputPath(outputFile);
        std::filesystem::create_directories(outputPath.parent_path());
    }

    // Write packfile
    if ((error = git_packbuilder_write(pb, outputFile.c_str(), 0, NULL, NULL)) != 0) {
        std::cerr << "Failed to write packfile\n";
        goto cleanup_all;
    }

    std::cout << "Successfully created packfile: " << outputFile << "\n";

cleanup_all:
    if (pb) git_packbuilder_free(pb);
    if (sig) git_signature_free(sig);
    if (tree) git_tree_free(tree);

cleanup:
    if (index) git_index_free(index);
    if (repo) git_repository_free(repo);
    git_libgit2_shutdown();

    return error;
}