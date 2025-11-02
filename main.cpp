#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <openssl/sha.h> 
#include <vector>
#include "zstr.hpp"
#include <sys/stat.h>
#include <algorithm>

using namespace std;
using namespace filesystem;

string generateSHA1FromData(const string &input) 
{
    unsigned char hashBuffer[SHA_DIGEST_LENGTH];
    SHA_CTX shaState;
    SHA1_Init(&shaState);
    SHA1_Update(&shaState, input.c_str(), input.size());
    SHA1_Final(hashBuffer, &shaState);
    ostringstream hashStream;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) 
    {
        hashStream << hex << setw(2) << setfill('0') << static_cast<int>(hashBuffer[i]);
    }
    return hashStream.str();
}


string computeObjectHash(const string &inputFilePath, bool saveToFile = false) 
{
    path myGitFolder = ".mygit";


    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return "";
    }

    ifstream file(inputFilePath, ios::binary);
    if (!file.is_open()) 
    {
        cerr << "Error: Unable to open the specified file " << inputFilePath << "\n";
        return "";
    }

    ostringstream contentStream;
    contentStream << file.rdbuf();
    string content = contentStream.str();

    // Generate the SHA-1 hash of the file's contents
    string hashValue = generateSHA1FromData(content);

    if (saveToFile) 
    {
        // Specify the path for the object file
        path outputPath = ".mygit/objects/" + hashValue.substr(0, 2) + "/" + hashValue.substr(2);
        if (!exists(outputPath.parent_path())) 
        {
            create_directories(outputPath.parent_path());
        }

        // Use zstr::ofstream to compress and save the file content
        zstr::ofstream output(outputPath.string(), ios::binary);
        output << content;
    }

    return hashValue;
}

void showFile(const string &argument, const string &sha1Hash) 
{
    path myGitFolder = ".mygit";

    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return;
    }

    path objectFilePath = myGitFolder / "objects" / sha1Hash.substr(0, 2) / sha1Hash.substr(2);
    if (!exists(objectFilePath)) 
    {
        cerr << "Error: Unable to locate the object with SHA-1 " << sha1Hash << ".\n";
        return;
    }

    if (argument == "-p") 
    {  
        zstr::ifstream fileStream(objectFilePath, ios::binary);
        if (fileStream.is_open()) 
        {
            cout << fileStream.rdbuf() << "\n";
        } 
        else 
        {
            cerr << "Error: Cannot read the content of the file.\n";
        }
    } 
    else if (argument == "-s") 
    {  

        cout << "Size of the file: " << file_size(objectFilePath) << " bytes\n";
    } 
    else if (argument == "-t") 
    {   

        zstr::ifstream fileStream(objectFilePath, ios::binary);
        if (!fileStream.is_open()) 
        {
            cerr << "Error: Cannot read the content of the file.\n";
            return;
        }

        // Read the first few bytes to check the format
        string content;
        getline(fileStream, content);

        // Check if the content matches the tree format
        // Tree format typically starts with a file mode (like 100644 or 040000)
        // followed by type and hash
        bool isTree = false;
        istringstream iss(content);
        string firstWord;
        if (iss >> firstWord) 
        {
            // Check if the first word is a valid file mode (6 digits)
            if (firstWord.length() == 6 && all_of(firstWord.begin(), firstWord.end(), ::isdigit)) 
            {
                isTree = true;
            }
        }

        cout << "Type of the object: " << (isTree ? "tree" : "blob") << "\n";
    } 
    else 
    {
        cerr << "Error: Invalid argument. Use '-p', '-s', or '-t'.\n";
    }
}

// Function to check if a file has executable permissions
bool checkIfExecutable(const string &fileName) 
{
    struct stat fileStatus;
    if (stat(fileName.c_str(), &fileStatus) != 0) 
    {
        return false; 
    }
    return (fileStatus.st_mode & S_IXUSR) != 0; 
}

string buildTree(const path &directoryPath) 
{

    path myGitFolder = ".mygit";


    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return "";
    }
    ostringstream treeStream;

    for (const auto &entry : directory_iterator(directoryPath)) 
    {
        string entryName = entry.path().filename().string();
        string sha1Hash;
        string permissions;

        if (entry.is_directory()) 
        {
            sha1Hash = buildTree(entry.path());
            permissions = "040000"; // Mode for directories
            treeStream << permissions << " tree " << sha1Hash << " " << entryName << "\n";
        } 
        else if (entry.is_regular_file()) 
        {
            sha1Hash = computeObjectHash(entry.path(), true);
            permissions = "100644"; // Default mode for normal files

            if (checkIfExecutable(entry.path().string())) 
            {
                permissions = "100755"; // Mode for executable files
            }
            treeStream << permissions << " blob " << sha1Hash << " " << entryName << "\n";
        }
    }

    string treeData = treeStream.str();
    string treeSHA1 = generateSHA1FromData(treeData);

    // Save the tree object with compression
    path objectPath = ".mygit/objects/" + treeSHA1.substr(0, 2) + "/" + treeSHA1.substr(2);
    if (!exists(objectPath.parent_path())) 
    {
        create_directories(objectPath.parent_path());
    }

    ofstream outputFile(objectPath, ios::binary);
    zstr::ostream compressedOutput(outputFile); // Use zstr to compress the data
    compressedOutput << treeData;

    return treeSHA1;
}

void listTreeContents(const string &sha, bool showNamesOnly) 
{
    path myGitFolder = ".mygit";

    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return;
    }
    if (sha.size() != 40) 
    {
        cerr << "Error: Invalid SHA-1 hash provided.\n";
        return;
    }

    path objectPath = ".mygit/objects/" + sha.substr(0, 2) + "/" + sha.substr(2);

    if (!exists(objectPath)) 
    {
        cerr << "Error: Object with SHA-1 " << sha << " not found.\n";
        return;
    }

    ifstream objectFile(objectPath, ios::binary);
    if (!objectFile.is_open()) 
    {
        cerr << "Error: Could not open the object file.\n";
        return;
    }

    zstr::istream decompressedInput(objectFile); // Use zstr to decompress the data
    string currentLine;

    // Directly list tree contents
    while (getline(decompressedInput, currentLine)) 
    {
        istringstream stream(currentLine);
        string permissions, entryType, sha, entryName;
        stream >> permissions >> entryType >> sha >> entryName;

        if (showNamesOnly) 
        {
            cout << entryName << "\n";
        } else 
        {
            cout << permissions << " " << entryType << " " << sha << " " << entryName << "\n";
        }
    }
}



// Add command: Adds files to the staging area (index)
void addFiles(const vector<string>& file_paths) 
{
    path myGitFolder = ".mygit";


    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return;
    }
    path index_path = ".mygit/index";

    ofstream index_file(index_path, ios::app);  // Open in append mode to add new entries
    if (!index_file.is_open()) 
    {
        cerr << "Error: Could not open index file.\n";
        return;
    }

    for (const string& file_path : file_paths) 
    {
        if (is_regular_file(file_path)) 
        {
            string sha1_hash = computeObjectHash(file_path, true);
            if (!sha1_hash.empty()) 
            {
                index_file << sha1_hash << " " << file_path << "\n";
                cout << "Added " << file_path << " to index.\n";
            }
        } 
        else if (is_directory(file_path)) 
        {
            for (const auto& entry : recursive_directory_iterator(file_path)) 
            {
                if (is_regular_file(entry.path())) 
                {
                    string relative_path = relative(entry.path()).string();
                    string sha1_hash = computeObjectHash(relative_path, true);
                    if (!sha1_hash.empty()) 
                    {
                        index_file << sha1_hash << " " << relative_path << "\n";
                        cout << "Added " << relative_path << " to index.\n";
                    }
                }
            }
        } 
        else 
        {
            cerr << "Error: " << file_path << " is not a valid file or directory.\n";
        }
    }

    index_file.close();
}

void setupGitRepo() {
    path myGitFolder = ".mygit";
    path objectsFolder = myGitFolder / "objects";
    path refsFolder = myGitFolder / "refs";
    path headFilePath = myGitFolder / "HEAD";
    path indexFilePath = myGitFolder / "index";  


    // Check if the .mygit folder already exists
    if (exists(myGitFolder)) 
    {
        cout << "Repository has already been initialized.\n";

        // Check for necessary files and folders
        bool missing = false;

        if (!exists(objectsFolder)) 
        {
            cout << "Creating missing directory: " << objectsFolder << "\n";
            create_directory(objectsFolder);
            missing = true;
        }

        if (!exists(refsFolder)) 
        {
            cout << "Creating missing directory: " << refsFolder << "\n";
            create_directory(refsFolder);
            missing = true;
        }

        if (!exists(headFilePath)) 
        {
            cout << "Creating missing HEAD file: " << headFilePath << "\n";
            ofstream headFileStream(headFilePath);
            if (headFileStream.is_open()) 
            {
                headFileStream << "ref: refs/heads/main\n";  // Initial reference for the main branch
                headFileStream.close();
            } else 
            {
                cerr << "Error: Failed to create HEAD file.\n";
                return;
            }
            missing = true;
        }

        if (!exists(indexFilePath)) 
        {
            cout << "Creating missing index file: " << indexFilePath << "\n";
            ofstream indexFileStream(indexFilePath);
            if (indexFileStream.is_open()) 
            {
                indexFileStream.close();
            } else 
            {
                cerr << "Error: Failed to create index file.\n";
                return;
            }
            missing = true;
        }

        if (!missing) 
        {
            cout << "All necessary files and folders are present.\n";
        } 
        else 
        {
            cout << "Missing files and folders have been created.\n";
        }

        return;
    }

// Create required directories
    try {
        create_directory(myGitFolder);
        create_directory(objectsFolder);
        create_directory(refsFolder);

        ofstream headFileStream(headFilePath);
        if (headFileStream.is_open()) 
        {
            headFileStream << "ref: refs/heads/main\n";  
            headFileStream.close();
        } 
        else 
        {
            cerr << "Error: Failed to create HEAD file.\n";
            return;
        }

        ofstream indexFileStream(indexFilePath);
        if (indexFileStream.is_open()) 
        {
            indexFileStream.close();
        } 
        else 
        {
            cerr << "Error: Failed to create index file.\n";
            return;
        }

        cout << "Initialized an empty Git repository located at " << absolute(myGitFolder) << "\n";
    } 
    catch (const exception &ex) 
    {
        cerr << "Error: " << ex.what() << "\n";
    }
}

// Struct to represent a Commit
struct Commit 
{
    string sha;
    string tree_sha;
    string message;
    string parent_sha;
    time_t timestamp;
};

Commit createCommit(const string &message, const string &tree_sha, const string &parent_sha) 
{
    Commit commit;
    commit.tree_sha = tree_sha;
    commit.message = message;
    commit.parent_sha = parent_sha;
    commit.timestamp = time(nullptr);

    // Get the username of the logged-in user
    const char* username = getlogin();
    string authorName = username ? username : "Unknown User";
    string authorEmail = authorName + "@students.iit.ac.in"; 

    // Create a time string for the commit timestamp
    std::ostringstream timeStream;
    std::tm localTime;
    localtime_r(&commit.timestamp, &localTime); 

 
    timeStream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");


    ostringstream commit_content;
    commit_content << "tree " << tree_sha << "\n";
    if (!parent_sha.empty()) 
    {
        commit_content << "parent " << parent_sha << "\n";
    }
    
    commit_content << "Committer " << authorName << " <" << authorEmail << ">\n" 
                   << "Timestamp "<<timeStream.str() << " +0530\n"; 
    commit_content << "Message "<< message << "\n"; 

    commit.sha = generateSHA1FromData(commit_content.str());

    path commit_path = ".mygit/objects/" + commit.sha.substr(0, 2) + "/" + commit.sha.substr(2);
    if (!exists(commit_path.parent_path())) 
    {
        create_directories(commit_path.parent_path());
    }

    ofstream commit_file(commit_path, ios::binary);
    commit_file << commit_content.str();

    return commit;
}



void commitChanges(const string &message) 
{
    path myGitFolder = ".mygit";


    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return;
    }
    // Check if there are any staged changes
    ifstream index_file(".mygit/index");
    if (!index_file.is_open()) 
    {
        cerr << "Error: Could not open index file.\n";
        return;
    }

    // Read the staged changes from the index
    string line;
    vector<string> staged_files;
    while (getline(index_file, line)) 
    {
        if (!line.empty()) 
        {
            size_t space_pos = line.find(' ');
            if (space_pos != string::npos) 
            {
                staged_files.push_back(line.substr(space_pos + 1)); 
            }
        }
    }

    // If no files are staged, exit without committing
    if (staged_files.empty()) 
    {
        cout << "No changes staged for commit.\n";
        return;
    }

    string parent_sha;
    ifstream head_file(".mygit/HEAD");
    if (head_file.is_open()) 
    {
        string ref;
        getline(head_file, ref);
        if (ref.substr(0, 4) == "ref:") 
        {
            ref = ref.substr(5);  
            ifstream ref_file(ref);
            if (ref_file.is_open()) 
            {
                getline(ref_file, parent_sha);
            }
        } 
        else 
        {
            parent_sha = ref; 
        }
    } 
    else 
    {
        cerr << "Error: Could not read HEAD file.\n";
        return;
    }

    // Get the tree SHA from the writeTree function
    string tree_sha = buildTree(".");

    // Create a commit
    Commit commit = createCommit(message, tree_sha, parent_sha);
    cout << "Created commit: " << commit.sha << "\n";

    // Update HEAD to point to the new commit
    ofstream head_output(".mygit/HEAD");
    head_output << commit.sha << "\n";

    // Clear the index after commit
    ofstream index_output(".mygit/index", ios::trunc);
    index_output.close();
}


void logCommits() 
{
    path myGitFolder = ".mygit";


    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return;
    }
    string head;
    ifstream head_file(".mygit/HEAD");
    if (!head_file.is_open()) 
    {
        cerr << "Error: Could not open HEAD file.\n";
        return;
    }
    getline(head_file, head);
    if (head.substr(0, 4) == "ref:") 
    {
        head = head.substr(5);  
        ifstream ref_file(head);
        if (ref_file.is_open()) 
        {
            getline(ref_file, head);
        } 
        else 
        {
            cerr << "Error: Could not open reference file.\n";
            return;
        }
    }

    // Start logging from the latest commit SHA
    string current_sha = head;
    while (!current_sha.empty()) 
    {
        path commit_path = ".mygit/objects/" + current_sha.substr(0, 2) + "/" + current_sha.substr(2);
        if (!exists(commit_path)) 
        {
            cerr << "Error: Commit object with SHA " << current_sha << " not found.\n";
            return;
        }

        // Use zstr to read the compressed commit file
        zstr::ifstream commit_file(commit_path, ios::binary);
        if (!commit_file.is_open()) 
        {
            cerr << "Error: Could not open commit file for SHA " << current_sha << ".\n";
            return;
        }

        string line;
        string commit_timestamp, parent_sha, committer_info,commit_message;
        while (getline(commit_file, line)) 
        {
            if (line.find("parent") == 0) 
            {
                parent_sha = line.substr(7);  // Get parent SHA
            } 
            else if (line.find("Committer") == 0) 
            {
                committer_info = line.substr(10);  // Save committer information
            }
            else if (line.find("Timestamp") == 0) {
                commit_timestamp = line.substr(10);  // Save timestamp information
            }
            else if (line.find("Message") == 0) 
            {
                commit_message = line.substr(8);  // Save message information
            }
            else if (line.empty()) 
            {
                break;  
            }
        }
        


        // Output formatted commit information
        cout << "Commit: " << current_sha << "\n";
        if (!parent_sha.empty()) 
        {
            cout << "Parent: " << parent_sha << "\n";
        }
        cout << "Committer: " << committer_info << "\n"; 
        cout << "Message: " << commit_message << "\n";
        cout << "Timestamp: " <<commit_timestamp << "\n\n";

        current_sha = parent_sha; 
    }
}


struct TreeEntry 
{
    string permissions;
    string type;
    string sha;
    string name;
};


// Function to restore files from a tree object recursively
void restoreFromTree(const string &treeSha, const path &currentPath) 
{
    path treePath = ".mygit/objects/" + treeSha.substr(0, 2) + "/" + treeSha.substr(2);
    if (!exists(treePath)) 
    {
        throw runtime_error("Tree object not found: " + treeSha);
    }

    // Read and decompress tree object
    zstr::ifstream treeFile(treePath, ios::binary);
    string line;
    
    while (getline(treeFile, line)) 
    {
        TreeEntry entry;
        istringstream iss(line);
        iss >> entry.permissions >> entry.type >> entry.sha >> entry.name;
        
        path entryPath = currentPath / entry.name;

        if (entry.type == "blob") 
        {
            create_directories(entryPath.parent_path());
            
            path blobPath = ".mygit/objects/" + entry.sha.substr(0, 2) + "/" + entry.sha.substr(2);
            zstr::ifstream blobFile(blobPath, ios::binary);
            ofstream outputFile(entryPath, ios::binary);
            
            outputFile << blobFile.rdbuf();
            outputFile.close();
            
            if (entry.permissions == "100755") 
            {
                chmod(entryPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            } 
            else 
            {
                chmod(entryPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            }
        } 
        else if (entry.type == "tree") 
        {
            create_directories(entryPath);
            restoreFromTree(entry.sha, entryPath);
        }
    }
}


void cleanWorkingDirectory() 
{
    for (const auto &entry : directory_iterator(".")) 
    {
        string entryName = entry.path().filename().string();
        // Skip .mygit directory and other special files/directories
        if (entryName != ".mygit" && entryName != "." && entryName != "..") 
        {
            remove_all(entry.path());
        }
    }
}

void checkoutCommit(const string &commitSha) 
{
    path myGitFolder = ".mygit";


    if (!exists(myGitFolder)) 
    {
        cerr << "Error: Git hasn't been initialized yet." << "\n";
        return;
    }
    try 
    {
        // Validate commit SHA format
        if (commitSha.length() != 40) 
        {
            throw runtime_error("Invalid commit SHA format");
        }

        // Read commit object
        path commitPath = ".mygit/objects/" + commitSha.substr(0, 2) + "/" + commitSha.substr(2);
        if (!exists(commitPath)) 
        {
            throw runtime_error("Commit not found: " + commitSha);
        }

        // Read and decompress commit object
        zstr::ifstream commitFile(commitPath, ios::binary);
        string line, treeSha;
        
        // Extract tree SHA from commit
        while (getline(commitFile, line)) 
        {
            if (line.find("tree ") == 0) 
            {
                treeSha = line.substr(5);
                break;
            }
        }

        if (treeSha.empty()) 
        {
            throw runtime_error("No tree found in commit");
        }
        
        cleanWorkingDirectory();

        restoreFromTree(treeSha, ".");

        // Update HEAD to point to the new commit
        ofstream headFile(".mygit/HEAD", ios::trunc);
        headFile << commitSha << "\n";
        headFile.close();

        cout << "Successfully checked out commit " << commitSha << "\n";

    } 
    catch (const exception &e) 
    {
        cerr << "Error during checkout: " << e.what() << "\n";
    }
}


int main(int argc, char *argv[]) 
{
    if (argc < 2) {
        cerr << "Error: No command provided.\n";
        return 1;
    }

    string command = argv[1];

    if (command == "init") 
    {
        setupGitRepo();
    } 
    else if (command == "hash-object") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        if (argc < 3) 
        {
            cerr << "Error: Missing arguments for hash-object.\n";
            return 1;
        }
        if (argc > 3 && string(argv[2]) != "-w") 
        {
            cerr << "Invalid Flag.\n";
            return 1;
        }
        bool write = (argc > 3 && string(argv[2]) == "-w");
        string file_path = (write) ? argv[3] : argv[2];
        string sha1_hash = computeObjectHash(file_path, write);
        if (!sha1_hash.empty()) 
        {
            cout << "SHA-1 hash: " << sha1_hash << "\n";
        }
    } 
    else if (command == "cat-file") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        if (argc < 4) 
        {
            cerr << "Error: Missing arguments for cat-file.\n";
            return 1;
        }
        string flag = argv[2];
        string file_sha = argv[3];
        showFile(flag, file_sha);
    } 
    else if (command == "write-tree") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        if (argc > 2) 
        {
            cerr << "Error: Additional arguments given for write-tree.\n";
            return 1;
        }
        string tree_sha1 = buildTree(".");
        if(tree_sha1!="")
        {
            cout << "Tree SHA-1: " << tree_sha1 << "\n";
        }
        
    } 
    else if (command == "ls-tree") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        if (argc < 3) 
        {
            cerr << "Error: Missing arguments for ls-tree.\n";
            return 1;
        }
        bool name_only = (argc > 3 && string(argv[2]) == "--name-only");
        string tree_sha = name_only ? argv[3] : argv[2];
        listTreeContents(tree_sha, name_only);
    } 
    else if (command == "add") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        vector<string> file_paths;
        for (int i = 2; i < argc; ++i) 
        {
            file_paths.push_back(argv[i]);
        }
        addFiles(file_paths);
    } 
    else if (command == "commit") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        string message = "Default commit message";  // Default message
        if (argc >= 4 && string(argv[2]) == "-m") 
        {
            message = argv[3];  // Use provided message
        }
        else if (argc >= 3 && string(argv[2]) != "-m")
        {
            cerr << "Invalid Flag.";
            return 1;
        }
        commitChanges(message);
    }
    else if (command == "log") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        if (argc > 2) 
        {
            cerr << "Error: Additional arguments given for log.\n";
            return 1;
        }
        logCommits();
    } 
    else if (command == "checkout") 
    {
        path myGitFolder = ".mygit";


        // Check if the .mygit folder doesn't exist
        if (!exists(myGitFolder)) 
        {
            cerr << "Error: Git hasn't been initialized yet." << "\n";
            return 0;
        }
        if (argc < 3) 
        {
            cerr << "Error: Missing commit SHA.\n";
            return 1;
        }
        if (argc > 3) 
        {
            cerr << "Error: Additional arguments given for Checkout.\n";
            return 1;
        }
        string commit_sha = argv[2];
        checkoutCommit(commit_sha);
    } 
    else if (command == "exit") 
    {
        cout << "Exiting program.\n";
        return 0;
    } 
    else 
    {
        cerr << "Invalid command.\n";
    }

    return 0;
}
