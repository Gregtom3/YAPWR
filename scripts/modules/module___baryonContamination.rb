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
  log_file = File.join(outdir, "#{tag}_baryonContamination.yaml")

  # Build ROOT macro invocation with log path argument
  macro = %Q{src/baryonContamination.C("#{filtered_tfile}","#{tree_name}","#{log_file}")}
  cmd   = ['root', '-l', '-b', '-q', macro]

  puts "[baryonContamination][#{tag}] #{filtered_tfile} (#{tree_name})"
  puts "  -> writing results to #{log_file}"

  # Run macro (it writes directly to log_file)
  unless system(*cmd)
    STDERR.puts "[baryonContamination] ERROR: ROOT macro failed with exit status #{$?.exitstatus}"
  end
end
