#!/usr/bin/env ruby
require 'fileutils'
require 'yaml'

# module___filterTree.rb
#
# Usage:
#   ruby module___filterTree.rb PROJECT_NAME
#
# Expects an "out/PROJECT_NAME" tree structured as:
#   out/PROJECT_NAME/
#     config_<CONFIG_NAME>/
#       <original_config>.yml
#       volatile_project.txt
#       <pair>/
#         <tag>/
#           tree_info.yaml
#
# For each tree_info.yaml, reads:
#   tfile: /abs/path/to/input.root
#   ttree: dihadron_cuts_noPmin
#
# and invokes:
#   root -l -b -q ".L src/filterTree.C+; filterTree(tfile,ttree,config_path,pair,output_dir);"

project_name = ARGV.fetch(0) do
  STDERR.puts "Usage: #{$0} PROJECT_NAME"
  exit 1
end

out_root = File.join("out", project_name)
unless Dir.exist?(out_root)
  STDERR.puts "ERROR: output root '#{out_root}' does not exist"
  exit 1
end

Dir.glob(File.join(out_root, "config_*")).sort.each do |config_dir|
  # find the original config file in the top of this dir
  config_file = Dir.glob(File.join(config_dir, "*.yaml")).
                  reject { |f| File.basename(f) == "tree_info.yaml" }.first
  unless config_file
    STDERR.puts "WARNING: no config .yaml in #{config_dir}, skipping"
    next
  end

  puts "\n== Processing #{File.basename(config_dir)} =="

  # for every tree_info.yaml in the subtree
  Dir.glob(File.join(config_dir, "**", "tree_info.yaml")).sort.each do |info_path|
    info = YAML.load_file(info_path)
    tfile = info.fetch("tfile")
    ttree = info.fetch("ttree")

    # derive pair and tag from directory structure
    leaf_dir = File.dirname(info_path)
    tag      = File.basename(leaf_dir)
    pair     = File.basename(File.dirname(leaf_dir))

    # prepare the output directory for filterTree
    output_dir = leaf_dir

    # build the ROOT command
    root_cmd = "root -l src/filterTree.C\\(\\\"#{tfile}\\\",\\\"#{ttree}\\\",\\\"#{config_file}\\\",\\\"#{pair}\\\",\\\"#{output_dir}\\\"\\)"
    
    puts "Running filterTree on #{tfile} (tree=#{ttree}) -> #{output_dir}"
    puts root_cmd
    puts "\n"
    #system("root", "-l", "-b", "-q", root_cmd) or
    #  STDERR.puts("ERROR: filterTree failed for #{info_path}")
  end
end
