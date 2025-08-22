#include "unpacker.h"
#include "git2.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>

// Helper function to clean up a temporary directory/file
static void cleanup_path(const std::string& path) {
    try {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to remove temporary path: " << e.what() << std::endl;
    }
}

int extract_packfile(const std::string& packFile, const std::string& outputDir) {
    git_libgit2_init();

    int error = 0;
    git_repository* repo = nullptr;
    git_odb* odb = nullptr;
    git_commit* commit = nullptr;
    git_tree* tree = nullptr;
    git_odb_backend* pack_backend = nullptr;
    std::string temp_pack_path;
    std::string temp_repo_path;

    // 1. Read the commit OID from the end of the packfile
    char oid_str[GIT_OID_HEXSZ + 1] = {0};
    {
        std::ifstream infile(packFile, std::ios::binary | std::ios::ate);
        if (!infile.is_open()) {
            std::cerr << "Failed to open packfile: " << packFile << std::endl;
            return -1;
        }
        std::streampos size = infile.tellg();
        if (size < GIT_OID_HEXSZ) {
            std::cerr << "Invalid packfile: file is too small" << std::endl;
            return -1;
        }
        infile.seekg(size - GIT_OID_HEXSZ);
        infile.read(oid_str, GIT_OID_HEXSZ);
        infile.close();
    }

    // 2. Create a temporary copy of the packfile without the OID
    try {
        temp_pack_path = (std::filesystem::temp_directory_path() / "gpack_temp.pack").string();
        std::ifstream src(packFile, std::ios::binary);
        std::ofstream dst(temp_pack_path, std::ios::binary | std::ios::trunc);
        std::streamsize size = std::filesystem::file_size(packFile);
        std::vector<char> buffer(1024 * 1024); // 1MB buffer
        std::streamsize remaining = size - GIT_OID_HEXSZ;
        while (remaining > 0) {
            std::streamsize to_read = std::min(remaining, (std::streamsize)buffer.size());
            src.read(buffer.data(), to_read);
            dst.write(buffer.data(), src.gcount());
            remaining -= src.gcount();
        }
        src.close();
        dst.close();
    } catch (const std::exception& e) {
        std::cerr << "Failed to create temporary packfile: " << e.what() << std::endl;
        cleanup_path(temp_pack_path);
        return -1;
    }

    // 3. Create a bare temporary repository
    try {
        temp_repo_path = (std::filesystem::temp_directory_path() / "gpack_temp_repo").string();
        cleanup_path(temp_repo_path); // Clean up from previous runs if necessary
        if ((error = git_repository_init(&repo, temp_repo_path.c_str(), true)) != 0) {
            throw std::runtime_error("Failed to initialize temporary repository");
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        const git_error *err = git_error_last();
        if (err) std::cerr << "libgit2 error: " << err->message << std::endl;
        error = -1;
        goto cleanup;
    }

    // 4. Add the temporary packfile to the repository's ODB
    if ((error = git_repository_odb(&odb, repo)) != 0) {
        std::cerr << "Failed to get repository ODB" << std::endl;
        goto cleanup;
    }
    if ((error = git_odb_backend_one_pack(&pack_backend, temp_pack_path.c_str())) != 0) {
        std::cerr << "Failed to create packfile backend" << std::endl;
        goto cleanup;
    }
    if ((error = git_odb_add_backend(odb, pack_backend, 1)) != 0) {
        std::cerr << "Failed to add packfile backend to ODB" << std::endl;
        goto cleanup;
    }

    // 5. Convert OID string to git_oid and lookup the commit
    git_oid oid;
    if ((error = git_oid_fromstr(&oid, oid_str)) != 0) {
        std::cerr << "Invalid commit OID in packfile" << std::endl;
        goto cleanup;
    }
    if ((error = git_commit_lookup(&commit, repo, &oid)) != 0) {
        std::cerr << "Failed to lookup commit" << std::endl;
        const git_error *err = git_error_last();
        if (err) std::cerr << "libgit2 error: " << err->message << std::endl;
        goto cleanup;
    }

    // 6. Get the tree from the commit
    if ((error = git_commit_tree(&tree, commit)) != 0) {
        std::cerr << "Failed to get tree from commit" << std::endl;
        goto cleanup;
    }

    // 7. Checkout the tree to the output directory
    {
        std::filesystem::create_directories(outputDir);
        git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        opts.checkout_strategy = GIT_CHECKOUT_FORCE;
        opts.target_directory = outputDir.c_str();

        if ((error = git_checkout_tree(repo, (const git_object*)tree, &opts)) != 0) {
            std::cerr << "Failed to checkout tree" << std::endl;
            const git_error *err = git_error_last();
            if (err) std::cerr << "libgit2 error: " << err->message << std::endl;
            goto cleanup;
        }
    }

    std::cout << "Successfully extracted packfile to: " << outputDir << std::endl;

cleanup:
    if (tree) git_tree_free(tree);
    if (commit) git_commit_free(commit);
    // The pack_backend is owned by the ODB after being added, so we don't free it.
    if (odb) git_odb_free(odb);
    if (repo) git_repository_free(repo);

    // Clean up temporary files and directories
    cleanup_path(temp_pack_path);
    cleanup_path(temp_repo_path);

    git_libgit2_shutdown();
    return error;
}