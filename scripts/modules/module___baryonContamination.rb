#!/usr/bin/env ruby
require 'yaml'
require 'fileutils'

project = ARGV.fetch(0) do
  STDERR.puts "Usage: #{$0} PROJECT_NAME"
  exit 1
end

out_root = File.join("out", project)
unless Dir.exist?(out_root)
  STDERR.puts "ERROR: '#{out_root}' does not exist"
  exit 1
end

Dir.glob(File.join(out_root, "config_*", "**", "tree_info.yaml")).sort.each do |info_path|
  tag = File.basename(File.dirname(info_path))
  next unless tag.include?("MC")

  info       = YAML.load_file(info_path)
  orig_tfile = info.fetch("tfile")
  tree_name  = info.fetch("ttree")
    
  # build the path to the *filtered* file
  leaf_dir     = File.dirname(info_path)
  filtered_tfile = File.join(leaf_dir, File.basename(orig_tfile))

  unless File.exist?(filtered_tfile)
    STDERR.puts "[baryonContamination][#{tag}] WARNING: filtered file not found: #{filtered_tfile}"
    next
  end
    
  # Prepare output directory and log file
  outdir   = File.join(leaf_dir, 'module-out___baryonContamination')
  FileUtils.mkdir_p(outdir)
  log_file = File.join(outdir, "baryonContamination.yaml")

  # Ascend from leaf to the config_<NAME> dir
  cfg_dir   = leaf_dir
  cfg_dir   = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?("config_")
  cfg_name  = File.basename(cfg_dir).sub(/^config_/, '')
  primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")

  # Build ROOT macro invocation with log path argument
  macro = %Q{src/modules/baryonContamination.C("#{orig_tfile}","#{tree_name}","#{primary_yaml}","#{log_file}")}
  cmd   = ['root', '-l', '-b', '-q', macro]

  puts "[baryonContamination][#{tag}] #{orig_tfile} (#{tree_name})"
  puts "  -> writing results to #{log_file}"

  # Run macro (it writes directly to log_file)
  unless system(*cmd)
    STDERR.puts "[baryonContamination] ERROR: ROOT macro failed with exit status #{$?.exitstatus}"
  end
end
