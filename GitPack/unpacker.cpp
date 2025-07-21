#include "unpacker.h"
#include <git2.h>
#include <iostream>
#include <filesystem>

int extract_packfile(const std::string& packFile, const std::string& outputDir) {
    git_libgit2_init();

    git_repository* repo = nullptr;
    int error = 0;

    // Create output directory
    std::filesystem::create_directories(outputDir);

    // Initialize a temporary repository in output directory
    std::string tempRepoPath = outputDir + "/.temp_repo";
    if ((error = git_repository_init(&repo, tempRepoPath.c_str(), false)) != 0) {
        std::cerr << "Failed to init temporary repo\n";
        const git_error* e = git_error_last();
        if (e) std::cerr << "Error: " << e->message << "\n";
        git_libgit2_shutdown();
        return -1;
    }

    // Get repository's object database
    git_odb* odb;
    if ((error = git_repository_odb(&odb, repo)) != 0) {
        std::cerr << "Failed to get repository ODB\n";
        goto cleanup;
    }

    // Add the packfile to the ODB
    if ((error = git_odb_add_disk_alternate(odb, packFile.c_str())) != 0) {
        std::cerr << "Failed to add packfile to ODB\n";
        // Try alternative method - this is a simplified approach
        // In a real implementation, you'd need to parse the pack file manually
        std::cerr << "Note: Full pack extraction requires more complex implementation\n";
        goto cleanup;
    }

    std::cout << "Packfile extraction initiated. Output directory: " << outputDir << "\n";
    std::cout << "Note: This is a simplified implementation. Full extraction requires pack parsing.\n";

cleanup:
    if (odb) git_odb_free(odb);
    if (repo) git_repository_free(repo);
    
    // Clean up temporary repo
    std::filesystem::remove_all(tempRepoPath);
    
    git_libgit2_shutdown();
    return error;
}