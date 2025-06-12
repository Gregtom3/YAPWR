#!/usr/bin/env ruby
require 'yaml'
require 'fileutils'

# module___asymmetry.rb
#
# Usage:
#   ruby module___asymmetry.rb PROJECT_NAME
#
# Traverses:
#   out/PROJECT_NAME/config_<CONFIG>/.../<pair>/<tag>/tree_info.yaml
# For each tree_info.yaml it will:
#  1) Read the original tfile path and ttree name
#  2) Derive the filtered ROOT file path in that leaf dir
#  3) Invoke the asymmetry macro on it

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

    leaf_dir      = File.dirname(info_path)
    pair          = File.basename(File.dirname(leaf_dir))
    filtered_file = File.join(leaf_dir, File.basename(orig_tfile))

    unless File.exist?(filtered_file)
      STDERR.puts "[module___asymmetry] WARNING: filtered file not found: #{filtered_file}"
      next
    end

    # call the asymmetry macro
    root_cmd = [
      "root", "-l", "-q",
      "src/asymmetry.C\\(\\\"#{filtered_file}\\\",\\\"#{ttree}\\\",\\\"#{pair}\\\"\\)"
    ].join(' ')

    puts "[module___asymmetry] Running: #{root_cmd}"
    system(root_cmd) or
      STDERR.puts "[module___asymmetry] ERROR: asymmetry failed for #{info_path}"
  end
end
