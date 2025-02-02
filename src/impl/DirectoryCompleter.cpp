#include "DirectoryCompleter.h"

#include "PathMap.h"
#include "nlohmann/json.hpp"

#include <memory>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
namespace fs = std::filesystem;

DirectoryCompleter::DirectoryCompleter(const DCArgs& args) {
    // If arguments are not empty, override the default values
    if (!args.init_path.empty())
      init_path = std::move(args.init_path);
    if (!args.cache_path.empty())
      this->cache_path = std::move(args.cache_path);
		else
			this->cache_path = get_cache_path();
    if (!args.exclude.empty())
      exclusion_rules = std::move(args.exclude);
    
    if (args.build)
      collect_directories();
    else
      load();
}

void DirectoryCompleter::collect_directories() {
	try {
		fs::recursive_directory_iterator it(init_path, fs::directory_options::skip_permission_denied);
		fs::recursive_directory_iterator end;
		
		while (it != end) {
			const auto& entry = *it;

			// Get the deepest directory name
			std::string dir_name = get_deepest_dir(entry.path().string());

			// If the entry is a directory and it's not in the exclude list, add it to the PathMap
			if (fs::is_directory(entry)) {
				if (dir_name != "" && !should_exclude(dir_name)) {
					directories.add(entry.path().string(), dir_name);
				
				// Otherwise, don't add it to the PathMap and disable recursion into it's children
				} else {
					it.disable_recursion_pending();
				}
			}

			++it;
		}
		
	} catch (const fs::filesystem_error& e) {
		std::cerr << e.what() << std::endl;
	}
}

void DirectoryCompleter::save() const {
	json j;
	for (const auto& [dir, cache] : directories.map) {
		json entry;
		entry["dir"] = dir;
		

		ordered_json paths = ordered_json::array();
		for (const auto& path : cache.get_all_paths())
			paths.push_back(path);
		entry["paths"] = paths;

		j.push_back(entry);
	}
	std::ofstream file(cache_path);
	if (file.is_open()) {
		file << j.dump(4);
		file.close();
	} else {
		std::cerr << "Unable to open file for writing: " << cache_path << std::endl;
	}
}

void DirectoryCompleter::load() {
	std::ifstream file(cache_path);
	if (file.is_open()) {
		json j;
		file >> j;
		file.close();

		for (const auto& entry : j) {
			std::string dir = entry["dir"];
			ordered_json paths = entry["paths"];

			for (const auto& path : paths)
				directories.add(path.get<std::string>(), dir);
		}
	} else
		std::cerr << "Unable to open file for reading: " << cache_path << std::endl;
}

const std::shared_ptr<DoublyLinkedList> DirectoryCompleter::get_list_for(const std::string& dir) const {
	return directories.get_list_for(dir);
}

std::vector<std::string> DirectoryCompleter::get_all_matches(const std::string& dir) const {
	return directories.get_all_paths(dir);
}

bool DirectoryCompleter::should_exclude(const std::string& dirname) const {
	// Check if the directory should be excluded based on the exclusion rules
	for (const auto& rule : exclusion_rules) {
		switch (rule.type) {
      case ExclusionType::Exact:
        if (dirname == rule.pattern)
					return true;
				break;

      case ExclusionType::Prefix:
				if (dirname.size() >= rule.pattern.size() &&
						dirname.compare(0, rule.pattern.size(), rule.pattern) == 0)
					return true;
				break;
        
      case ExclusionType::Suffix:
				if (dirname.size() >= rule.pattern.size() &&
						dirname.compare(dirname.size() - rule.pattern.size(), rule.pattern.size(), rule.pattern) == 0)
					return true;
				break;
        
			case ExclusionType::Contains:
				if (dirname.find(rule.pattern) != std::string::npos)
					return true;
				break;
				
		}
	}

	return false;
}

std::string DirectoryCompleter::get_deepest_dir(const std::string& path) const {
	// Return the deepest directory name or "" if it is invalid
	size_t pos = path.find_last_of('/');
	if (pos >= std::string::npos)
		return "";

	std::string dir = path.substr(pos + 1);
	return dir;
}

std::string DirectoryCompleter::get_cache_path() const {
	const char* xdgCacheHome = std::getenv("XDG_CACHE_HOME");
	std::string cacheDir;

	if (xdgCacheHome) {
		cacheDir = std::string(xdgCacheHome) + "/dirvana/";
	} else {
		cacheDir = std::string(std::getenv("HOME")) + "/.cache/dirvana/";
	}

	// Ensure the directory exists
	std::filesystem::create_directories(cacheDir);

	return cacheDir + "cache.json";
}