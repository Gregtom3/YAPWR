#!/usr/bin/env ruby
# coding: utf-8
# module___binMigration.rb — invoke binMigration.C on each MC­-tagged leaf,
# passing it the filtered TFile, TTree name, the top‐level config YAML, and project root.

require 'yaml'
require 'fileutils'

project = ARGV.fetch(0) do
  STDERR.puts "Usage: #{$0} PROJECT_NAME"
  exit 1
end

out_root = File.join("out", project)
abort "ERROR: '#{out_root}' does not exist" unless Dir.exist?(out_root)

Dir
  .glob(File.join(out_root, "config_*", "**", "tree_info.yaml"))
  .sort
  .each do |info_path|

    tag = File.basename(File.dirname(info_path))
    next unless tag.include?("MC")
    
    info       = YAML.load_file(info_path)
    orig_tfile = info.fetch("tfile")
    tree_name  = info.fetch("ttree")
    leaf_dir   = File.dirname(info_path)
    filtered =   File.join(leaf_dir, "gen_#{File.basename(orig_tfile)}") # "gen_" added 07/18/2025

    unless File.exist?(filtered)
      STDERR.puts "[binMigration][#{tag}] WARNING: filtered file not found: #{filtered}"
      next
    end

    # Ascend from leaf to the config_<NAME> dir
    cfg_dir   = leaf_dir
    cfg_dir   = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?("config_")
    cfg_name  = File.basename(cfg_dir).sub(/^config_/, '')
    primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")

    unless File.exist?(primary_yaml)
      STDERR.puts "[binMigration][#{tag}] WARNING: primary YAML not found: #{primary_yaml}"
      next
    end
        
    # Prepare output directory and log file
    outdir   = File.join(leaf_dir, 'module-out___binMigration')
    FileUtils.mkdir_p(outdir)
    log_file = File.join(outdir, "binMigration.yaml")
        
    puts "[binMigration][#{tag}]"
    puts "    filtered:    #{filtered}"
    puts "    primary yaml: #{primary_yaml}"
    puts "    log file: #{log_file}"

    macro = %Q{src/modules/binMigration.C("#{orig_tfile}",
                                   "#{tree_name}",
                                   "#{primary_yaml}",
                                   "#{out_root}",
                                   "#{log_file}")}
    cmd = ['root','-l','-b','-q', macro]
    puts "  -> #{cmd.join(' ')}"
    system(*cmd) or STDERR.puts("[binMigration] ERROR on #{primary_yaml}")
  end
