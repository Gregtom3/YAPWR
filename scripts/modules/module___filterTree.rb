#!/usr/bin/env ruby
require 'optparse'
require 'yaml'
require 'fileutils'

# module___filterTree.rb
# Usage: ruby module___filterTree.rb [--slurm] [--dependency afterok:IDs] \
#                                     [--maxEntries N] PROJECT_NAME [CONFIG...]

options = { slurm: false, deps: nil, max_entries: -1 }
parser = OptionParser.new do |opts|
  opts.banner = "Usage: #{File.basename($0)} [--slurm] [--dependency afterok:IDs] [--maxEntries N] PROJECT_NAME [CONFIG...]"

  opts.on('--slurm', 'Submit jobs to Slurm via sbatch') do
    options[:slurm] = true
  end

  opts.on('--dependency D', 'SBATCH --dependency string') do |d|
    options[:deps] = d
  end

  opts.on('--maxEntries N', Integer, 'Limit max entries (positive only)') do |n|
    options[:max_entries] = n if n.positive?
  end
end
parser.parse!

project = ARGV.shift or abort(parser.banner)
user_configs = ARGV.map { |c| "config_#{File.basename(c, File.extname(c))}" }

out_root = File.join('out', project)
abort "ERROR: output root '#{out_root}' does not exist" unless Dir.exist?(out_root)

entry_arg = options[:max_entries]
puts "[INFO] maxEntries=#{entry_arg}" if entry_arg > 0
puts "[INFO] Filtering configs: #{user_configs.join(', ')}" unless user_configs.empty?

job_ids = []

Dir.glob(File.join(out_root, 'config_*')).sort.each do |cfg_dir|
  cfg_name = File.basename(cfg_dir)
  next if user_configs.any? && user_configs.none? { |c| cfg_name.include?(c) }

  puts "\n== Processing #{cfg_name} =="
  yaml_cfg = Dir.glob(File.join(cfg_dir, '*.yaml'))
                 .reject { |f| f.end_with?('tree_info.yaml') }
                 .first
  unless yaml_cfg
    warn "WARNING: no config .yaml in #{cfg_dir}, skipping"
    next
  end

  Dir.glob(File.join(cfg_dir, '**', 'tree_info.yaml')).sort.each do |info|
    data   = YAML.load_file(info)
    tfile  = data.fetch('tfile')
    ttree  = data.fetch('ttree')
    leaf   = File.dirname(info)
    tag    = File.basename(leaf)
    pair   = File.basename(File.dirname(leaf))
    outdir = leaf

    cmds = []
    cmds << %Q(root -l -q 'src/modules/filterTree.C("#{tfile}","#{ttree}","#{yaml_cfg}","#{pair}","#{outdir}",#{entry_arg})')
    if tag.start_with?('MC')
      cmds << %Q(root -l -q 'src/modules/filterTreeMC.C("#{tfile}","#{ttree}","#{yaml_cfg}","#{pair}","#{outdir}",#{entry_arg})')
    end

    if options[:slurm]
      job_name = "ft_#{pair}_#{tag}"
      slurm_script = <<~SLURM
        #!/bin/bash
        #SBATCH --job-name=#{job_name}
        #SBATCH --output=#{outdir}/filterTree.out
        #SBATCH --error=#{outdir}/filterTree.err
        #SBATCH --time=08:00:00
        #SBATCH --mem-per-cpu=2000
        #SBATCH --cpus-per-task=1
        #{options[:deps] ? "#SBATCH --dependency=#{options[:deps]}" : ''}
        cd #{Dir.pwd}
        #{cmds.join("\n")}
      SLURM

      script_path = File.join(outdir, 'run_filterTree.slurm')
      File.write(script_path, slurm_script)
      FileUtils.chmod('+x', script_path)

      output = `sbatch #{script_path}`
      if output =~ /Submitted batch job (\d+)/
        job_ids << Regexp.last_match(1)
        puts "[SLURM_JOBS] #{job_ids.join(',')}"
      else
        warn "ERROR: sbatch failed: #{output}"
      end
    else
      cmds.each do |cmd|
        puts "Running: #{cmd}"
        system(cmd) or warn "ERROR: filterTree failed for #{info}"
      end
    end
  end
end

puts "[SLURM_JOBS] #{job_ids.join(',')}" if options[:slurm] && job_ids.any?
