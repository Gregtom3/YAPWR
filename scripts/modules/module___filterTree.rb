#!/usr/bin/env ruby
require 'fileutils'
require 'yaml'

# module___filterTree.rb
# Usage: ruby module___filterTree.rb PROJECT_NAME [maxEntries]

project_name = ARGV.fetch(0) do
  STDERR.puts "Usage: #{$0} PROJECT_NAME [maxEntries]"
  exit 1
end
max_entries = ARGV[1] ? ARGV[1].to_i : nil

out_root = File.join("out", project_name)
unless Dir.exist?(out_root)
  STDERR.puts "ERROR: output root '#{out_root}' does not exist"
  exit 1
end

Dir.glob(File.join(out_root, "config_*")).sort.each do |config_dir|
  cfg = Dir.glob(File.join(config_dir, "*.yaml")).reject { |f| f.end_with?("tree_info.yaml") }.first
  unless cfg
    STDERR.puts "WARNING: no config .yaml in #{config_dir}, skipping"
    next
  end

  puts "\n== Processing #{File.basename(config_dir)} =="

  Dir.glob(File.join(config_dir, "**", "tree_info.yaml")).sort.each do |info_path|
    info  = YAML.load_file(info_path)
    tfile = info.fetch("tfile")
    ttree = info.fetch("ttree")

    leaf    = File.dirname(info_path)
    tag     = File.basename(leaf)
    pair    = File.basename(File.dirname(leaf))
    outdir  = leaf

    entry_arg = max_entries && max_entries > 0 ? max_entries : -1
    
    # build the ROOT command, inserting entry_arg at the end
    root_cmd = "root -l -q src/modules/filterTree.C\\(\\\"#{tfile}\\\",\\\"#{ttree}\\\",\\\"#{cfg}\\\",\\\"#{pair}\\\",\\\"#{outdir}\\\",#{entry_arg}\\)"
    
    puts "Running filterTree on #{tfile} (tree=#{ttree}) -> #{outdir}  [maxEntries=#{entry_arg}]"
      
    system(root_cmd) or
      STDERR.puts("ERROR: filterTree failed for #{info_path}")
  end
end
