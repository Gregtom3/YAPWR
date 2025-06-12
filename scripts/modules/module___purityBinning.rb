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
#  3) Invoke purityBinning on that filtered file and tree

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

    # derive the filtered file path by reusing the basename
    filtered_file = File.join(leaf_dir, File.basename(orig_tfile))
    unless File.exist?(filtered_file)
      STDERR.puts "[module___purityBinning] WARNING: filtered file not found: #{filtered_file}"
      next
    end

    # build the ROOT command exactly as specified
    root_cmd = %W[
      root -l -q
      src/purityBinning.C\\(\\\"#{filtered_file}\\\",\\\"#{ttree}\\\",\\\"#{pair}\\\"\\)
    ].join(' ')

    puts "[module___purityBinning] Running: #{root_cmd}"
    system(root_cmd) or
      STDERR.puts("[module___purityBinning] ERROR: purityBinning failed for #{info_path}")
  end
end
