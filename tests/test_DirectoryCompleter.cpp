#include <gtest/gtest.h>

#include "DirectoryCompleter.h"

#include <iostream>
#include <string>

using namespace std;

using ExclusionType = DirectoryCompleter::ExclusionType;
using ExclusionRule = DirectoryCompleter::ExclusionRule;

TEST(DirectoryCompleter, Initialization) {
    string root = "/Users/jameskendrick/Code/Projects/dirvana/mockfs";
    vector<ExclusionRule> exclude = {
			{ ExclusionType::Prefix, "."},
      { ExclusionType::Exact, "custom_rule_check" },
    };
    DirectoryCompleter completer(DCArgs{ .init_path = root, .exclude = exclude });

    EXPECT_EQ(completer.get_size(), 10);
}

void check(const string& root, const vector<string>& completions, const vector<string>& expected) {
	EXPECT_EQ(completions.size(), expected.size());
	for (int i = 0; i < completions.size(); i++) {
		EXPECT_EQ(completions[i], root + expected[i]);
	}
}

TEST(DirectoryCompleter, Completion) {
	string root = "/Users/jameskendrick/Code/Projects/dirvana/mockfs";
	vector<ExclusionRule> exclude = {
		{ ExclusionType::Prefix, "."},
    { ExclusionType::Exact, "custom_rule_check" },
  };
	DirectoryCompleter completer(DCArgs{ .init_path = root, .exclude = exclude });

	auto completions = completer.get_all_matches("0");
	check(root, completions, {});

	completions = completer.get_all_matches("1");
	check(root, completions, {"/1", "/1/1", "/1/1/1"});

	completions = completer.get_all_matches("2");
	check(root, completions, {"/2", "/2/2"});

	completions = completer.get_all_matches("3");
	check(root, completions, {"/3"});

	completions = completer.get_all_matches("4");
	check(root, completions, {"/1/1/1/4", "/4", "/3/4", "/2/2/4"});

}

TEST(DirectoryCompleter, Access) {
	string root = "/Users/jameskendrick/Code/Projects/dirvana/mockfs";
	vector<ExclusionRule> exclude = {
		{ ExclusionType::Prefix, "."},
    { ExclusionType::Exact, "custom_rule_check" },
  };
	DirectoryCompleter completer(DCArgs{.init_path = root, .exclude = exclude });

	completer.access(root + "/0");
	check(root, completer.get_all_matches("0"), {"/0"});

	completer.access(root + "/1/1");
	check(root, completer.get_all_matches("1"), {"/1/1", "/1", "/1/1/1"});

	completer.access(root + "/1/1/1");
	check(root, completer.get_all_matches("1"), {"/1/1/1", "/1/1", "/1"});

	completer.access(root + "/1/1/1");
	check(root, completer.get_all_matches("1"), {"/1/1/1", "/1/1", "/1"});
}

TEST(DirectoryCompleter, Exclusion) {
	string root = "/Users/jameskendrick/Code/Projects/dirvana/mockfs/custom_rule_check";
	vector<ExclusionRule> exclude = {
		{ ExclusionType::Prefix, "." },
	};
	DirectoryCompleter completer(DCArgs{ .init_path = root, .exclude = exclude });
	EXPECT_EQ(completer.get_size(), 4);

	exclude.push_back({ ExclusionType::Exact, "exact_check" });
	DirectoryCompleter completer2(DCArgs{ .init_path = root, .exclude = exclude });
	EXPECT_EQ(completer2.get_size(), 3);

	exclude.push_back({ ExclusionType::Contains, "ain" });
	DirectoryCompleter completer3(DCArgs{ .init_path = root, .exclude = exclude });
	EXPECT_EQ(completer3.get_size(), 2);

	exclude.push_back({ ExclusionType::Suffix, "eck" });
	DirectoryCompleter completer4(DCArgs{ .init_path = root, .exclude = exclude });
	EXPECT_EQ(completer4.get_size(), 0);
}

TEST(DirectoryCompleter, SaveAndLoad) {
	// Create a DirectoryCompleter with known data
	string root = "/Users/jameskendrick/Code/Projects/dirvana/mockfs";
	vector<ExclusionRule> exclude = {
		{ ExclusionType::Prefix, "."},
    { ExclusionType::Exact, "custom_rule_check" },
  };
	string cache_path = "test-cache.json";
	DirectoryCompleter original(DCArgs{ .init_path = root, .exclude = exclude, .cache_path = cache_path });

	// Access some paths
	original.access(root + "/1/1");
	original.access(root + "/2/2");
	original.access(root + "/2/2/4");

	// Save the completer state
	original.save();

	// Create a new completer instance and load the saved state
	DirectoryCompleter loaded(DCArgs{ false, root, exclude, cache_path });

	// Verify the loaded completer has the same matches
	auto loaded_matches_1 = loaded.get_all_matches("1");
	auto loaded_matches_2 = loaded.get_all_matches("2");
	auto loaded_matches_4 = loaded.get_all_matches("4");

	// Compare the results (reordering should persist)
	check(root, loaded_matches_1, {"/1/1", "/1", "/1/1/1"});
	check(root, loaded_matches_2, {"/2/2", "/2"});
	check(root, loaded_matches_4, {"/2/2/4", "/1/1/1/4", "/4", "/3/4"});

	// Verify that both completers have the same number of directories
	EXPECT_EQ(original.get_size(), loaded.get_size());
}