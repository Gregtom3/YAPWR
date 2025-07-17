#!/usr/bin/env ruby
require 'yaml'
require 'fileutils'

# module___purityBinning.rb
#
# Usage:
#   ruby module___purityBinning.rb PROJECT_NAME
#
# Traverses:
#   out/PROJECT_NAME/config_<CONFIG>/.../<pair>/<tag>/tree_info.yaml
# For each tree_info.yaml it will:
#  1) Read the original tfile path and ttree name
#  2) Build the path to the *filtered* ROOT file in that same leaf directory
#  3) Create module-out___purityBinning/ there
#  4) Invoke purityBinning on that filtered file, tree, pair, and output_dir

project_name = ARGV.fetch(0) {
  STDERR.puts "Usage: #{$0} PROJECT_NAME"
  exit 1
}

out_root = File.join("out", project_name)
unless Dir.exist?(out_root)
  STDERR.puts "ERROR: '#{out_root}' does not exist"
  exit 1
end

Dir.glob(File.join(out_root, "config_*")).sort.each do |config_dir|
  Dir.glob(File.join(config_dir, "**", "tree_info.yaml")).sort.each do |info_path|
    info       = YAML.load_file(info_path)
    orig_tfile = info.fetch("tfile")
    ttree      = info.fetch("ttree")

    leaf_dir   = File.dirname(info_path)
    pair       = File.basename(File.dirname(leaf_dir))
    next unless pair.include?("pi0")
    # 2) Derive and check the filtered file
    filtered_file = File.join(leaf_dir, File.basename(orig_tfile))
    unless File.exist?(filtered_file)
      STDERR.puts "[module___purityBinning] WARNING: filtered file not found: #{filtered_file}"
      next
    end

    # 3) Create the module-out directory
    output_dir = File.join(leaf_dir, "module-out___purityBinning")
    FileUtils.mkdir_p(output_dir)

    # 4) Build the ROOT command
    #    purityBinning(in, tree, pair, outDir)
    root_cmd = "root -l -q src/modules/purityBinning.C\\(\\\"#{filtered_file}\\\",\\\"#{ttree}\\\",\\\"#{pair}\\\",\\\"#{output_dir}\\\"\\)"

    puts "[module___purityBinning] Running: #{root_cmd}"
    system(root_cmd) or
      STDERR.puts("[module___purityBinning] ERROR: purityBinning failed for #{info_path}")
  end
end
