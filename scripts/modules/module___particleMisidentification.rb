#!/usr/bin/env ruby
# coding: utf-8
#
# module___particleMisidentification.rb – run particleMisidentification.C
# on every filtered tree in out/<PROJECT>, for MC-tagged leaves.

require 'yaml'
require 'fileutils'

# --- parse PROJECT_NAME ---
project = ARGV.fetch(0) do
  STDERR.puts "Usage: #{$0} PROJECT_NAME"
  exit 1
end

out_root = File.join("out", project)
unless Dir.exist?(out_root)
  STDERR.puts "ERROR: '#{out_root}' does not exist"
  exit 1
end

# --- scan all leaves ---
Dir.glob(File.join(out_root, "config_*", "**", "tree_info.yaml")).sort.each do |info_path|
  tag = File.basename(File.dirname(info_path))
  next unless tag.include?("MC")      # only MC-tagged subtrees

  info           = YAML.load_file(info_path)
  orig_tfile     = info.fetch("tfile")
  tree_name      = info.fetch("ttree")
  leaf_dir       = File.dirname(info_path)
  filtered_tfile = File.join(leaf_dir, File.basename(orig_tfile))
    
  unless File.exist?(filtered_tfile)
    STDERR.puts "[particleMisidentification][#{tag}] WARNING: filtered file not found: #{filtered_tfile}"
    next
  end

  # --- prepare output directory + yaml path ---
  outdir   = File.join(leaf_dir, "module-out___particleMisidentification")
  FileUtils.mkdir_p(outdir)
  yaml_path = File.join(outdir, "particleMisidentification.yaml")

  puts "[particleMisidentification][#{tag}] #{filtered_tfile} → #{yaml_path}"

 # Ascend from leaf to the config_<NAME> dir
 cfg_dir   = leaf_dir
 cfg_dir   = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?("config_")
 cfg_name  = File.basename(cfg_dir).sub(/^config_/, '')
 primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")
  # --- invoke ROOT macro with (file, tree, yaml_output) ---
  macro = %Q{src/modules/particleMisidentification.C("#{filtered_tfile}","#{tree_name}","#{primary_yaml}",#{yaml_path}")}
  cmd   = ["root", "-l", "-b", "-q", macro]

  puts "  -> #{cmd.map(&:inspect).join(' ')}"
  unless system(*cmd)
    STDERR.puts "[particleMisidentification] ERROR running: #{cmd.join(' ')}"
  end
end
