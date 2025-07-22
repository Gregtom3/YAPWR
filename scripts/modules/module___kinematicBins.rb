#!/usr/bin/env ruby
# coding: utf-8
# module___kinematicBins.rb   – run kinematicBins.C on every
#                                filtered tree in out/<PROJECT>,
#                                with optional Slurm submission

require 'yaml'
require 'fileutils'
require 'optparse'

# ------------------------------------------------------------------
#  CLI parsing
# ------------------------------------------------------------------
options = { slurm: false, deps: nil }
OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME"
  opts.on('--slurm', 'Submit each job via sbatch instead of running locally') do
    options[:slurm] = true
  end
  opts.on('--dependency D', 'SBATCH --dependency string (e.g. afterok:1234)') do |dep|
    options[:deps] = dep
  end
  opts.on('-h', '--help', 'Show this help message') do
    puts opts
    exit
  end
end.order!

project = ARGV.shift or abort("Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME")
out_root = File.join('out', project)
abort "ERROR: #{out_root} not found" unless Dir.exist?(out_root)

job_ids = []

Dir.glob(File.join(out_root, 'config_*')).sort.each do |cfg|
  Dir.glob(File.join(cfg, '**', 'tree_info.yaml')).sort.each do |tinfo|
    info  = YAML.load_file(tinfo)
    ttree = info['ttree']
    src   = info['tfile']
    leaf  = File.dirname(tinfo)
    froot = File.join(leaf, File.basename(src))     # filtered file
    next unless File.exist?(froot)

    pair   = File.basename(File.dirname(leaf))
    outdir = File.join(leaf, 'module-out___kinematicBins')
    FileUtils.mkdir_p(outdir)

    # ascend to config_<NAME>
    cfg_dir     = leaf
    cfg_dir     = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?('config_')
    cfg_name    = File.basename(cfg_dir).sub(/^config_/, '')
    primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")
    unless File.exist?(primary_yaml)
      warn "[kinematicBins][#{pair}] primary YAML not found: #{primary_yaml}"
      next
    end

    # prepare ROOT macro command
    macro = %Q{'src/modules/kinematicBins.C(
      "#{src}",
      "#{ttree}",
      "#{pair}",
      "#{primary_yaml}",
      "#{outdir}"
    )'}
    cmd = ['root', '-l', '-b', '-q', macro]

    if options[:slurm]
      # create sbatch script
      sbatch = <<~SLURM
        #!/bin/bash
        #SBATCH --job-name=kinBins_#{pair}
        #SBATCH --output=#{outdir}/kinematicBins_#{pair}.out
        #SBATCH --error=#{outdir}/kinematicBins_#{pair}.err
        #SBATCH --time=24:00:00
        #SBATCH --mem-per-cpu=1000
        #SBATCH --cpus-per-task=1
      SLURM
      sbatch += "#SBATCH --dependency=#{options[:deps]}\n" if options[:deps]
      sbatch += <<~SLURM
        cd #{Dir.pwd}
        #{cmd.join(' ')}
      SLURM

      script_file = File.join(outdir, "run_kinematicBins_#{pair}.slurm")
      File.write(script_file, sbatch)
      FileUtils.chmod('+x', script_file)

      # submit and capture ID
      out = `sbatch #{script_file}`
      if out =~ /Submitted batch job (\d+)\n?/ 
        job_ids << $1
        puts "[SLURM_JOBS] #{job_ids.join(',')}"
      else
        warn "[kinematicBins] sbatch failed for #{pair}: #{out.strip}"
      end
    else
      # direct local execution
      puts "[kinematicBins][#{pair}] RUN: #{cmd.join(' ')}"
      system(*cmd) or warn "[kinematicBins] ERROR running for #{pair}" 
    end
  end
end

# final print of all submitted job IDs
if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
