#!/usr/bin/env ruby
# coding: utf-8
require 'optparse'
require 'fileutils'
require 'yaml'
require 'shellwords'

# ------------------------------------------------------------------
#  User-constants
# ------------------------------------------------------------------
VOLATILE_PROJECT   = 'pipi0-paper-v5-pass2'
VOLATILE_SUFFIX    = '*merged_cuts_noPmin*'
VOLATILE_TREE_NAME = 'dihadron_cuts_noPmin'

# ------------------------------------------------------------------
#  CLI parsing
# ------------------------------------------------------------------
options = { append: false, maxEntries: nil, maxFiles: nil, slurm: false, is_running_on_slurm: true }
optlist  = []                                     # remember original flags

parser = OptionParser.new do |opts|
  opts.banner = <<~TXT
     Usage: #{$0} [options]  PROJECT  RUNCARD  CONFIG1 [CONFIG2 ...]
       --append           do not overwrite existing out/<PROJECT>; skip filterTree
       --maxEntries N     pass N to filterTree as entry limit
       --maxFiles   M     only create M tree_info.yaml per config
       --slurm            submit one Slurm job per CONFIG instead of running now
  TXT

  opts.on('--append')               { options[:append] = true ; optlist << '--append' }
  opts.on('--maxEntries N', Integer){ |n| options[:maxEntries] = n ; optlist += ['--maxEntries', n.to_s] }
  opts.on('--maxFiles M',  Integer){ |m| options[:maxFiles]   = m ; optlist += ['--maxFiles',   m.to_s] }
  opts.on('--slurm')                { options[:slurm]  = true }
  opts.on('--is_running_on_slurm')                { options[:is_running_on_slurm]  = true }
  opts.on('-h','--help'){ puts opts ; exit }
end

parser.parse!

# ------------------------------------------------------------------
#  Positional arguments
# ------------------------------------------------------------------
if ARGV.size < 3
  warn parser.banner
  exit 1
end

project_name = ARGV.shift
runcard_path = ARGV.shift
config_files = ARGV

abort "runcard '#{runcard_path}' not found" unless File.file?(runcard_path)

# ------------------------------------------------------------------
#  If --slurm → emit one batch file per config and exit
# ------------------------------------------------------------------
if options[:slurm]
  project_root = File.join('out', project_name)
  FileUtils.mkdir_p(project_root)

  config_files.each do |cfg_path|
    cfg_name  = File.basename(cfg_path, File.extname(cfg_path))
    cfg_dir   = File.join(project_root, "config_#{cfg_name}")
    FileUtils.mkdir_p(cfg_dir)

    # rebuild CLI: same flags except --slurm, and *only* this config
    cli = ['ruby',
           File.realpath(__FILE__),
           *optlist,
           project_name,
           "--is_running_on_slurm",
           File.realpath(runcard_path),
           File.realpath(cfg_path)
          ].shelljoin

    batch = <<~SLURM
      #!/bin/bash
      #SBATCH --job-name=#{project_name}_#{cfg_name}
      #SBATCH --output=#{project_root}/pipeline_#{cfg_name}.out
      #SBATCH --error=#{project_root}/pipeline_#{cfg_name}.err
      #SBATCH --time=24:00:00
      #SBATCH --mem-per-cpu=4000
      #SBATCH --cpus-per-task=4

      cd #{Dir.pwd}
      echo "== #{project_name}/#{cfg_name} started $(date)"
      #{cli}
      echo "== finished $(date)"
    SLURM

    slurm_script = File.join(cfg_dir, 'run_pipeline.slurm')
    File.write(slurm_script, batch)
    FileUtils.chmod('+x', slurm_script)

    puts "[run_project] sbatch #{slurm_script}"
    system('sbatch', slurm_script) or abort "sbatch failed for #{cfg_name}"
  end
  exit 0
end
# ------------------------------------------------------------------
#  Immediate (non-Slurm) run
# ------------------------------------------------------------------

# ---------- runcard ------------------------------------------------
runcard = YAML.load_file(runcard_path)
modules = runcard['modules']
abort "runcard needs an array 'modules'" unless modules.is_a?(Array)

# ---------- prepare out/ tree -------------------------------------
out_root = File.join('out', project_name)

# ---------- locate volatile ROOT files -----------------------------
base_dir = File.join('/volatile/clas12/users/gmat/clas12analysis.sidis.data',
                     'clas12_dihadrons','projects',VOLATILE_PROJECT,'data')
merged_files = Dir.glob(File.join(base_dir,'*',VOLATILE_SUFFIX))
abort "No merged_cuts files under #{base_dir}" if merged_files.empty?

# ---------- create config-specific dirs and tree_info.yaml ---------
config_files.each do |cfg|
  next unless File.file?(cfg)

  cfg_name = File.basename(cfg, File.extname(cfg))
  cfg_dir  = File.join(out_root, "config_#{cfg_name}")

  unless options[:append]
      if Dir.exist?(cfg_dir)
        #––– interactive only when we have a TTY –––
        answer =
          if $stdin.tty?            # local run: ask user
            print "Directory '#{cfg_dir}' exists. Overwrite? [y/N]: "
            STDIN.gets&.chomp&.downcase
          else                      # Slurm: no TTY → auto-yes
            'yes'
          end
    
        exit 0 unless %w[y yes].include?(answer)
        FileUtils.rm_rf(cfg_dir)
      end
  end
  FileUtils.mkdir_p(cfg_dir)
  FileUtils.cp(cfg, cfg_dir) unless options[:append]

  unless options[:append]
    File.write(File.join(cfg_dir, 'volatile_project.txt'), VOLATILE_PROJECT)

    usable = merged_files.reject { |f| File.basename(File.dirname(f)) == 'pi0_pi0' }
    usable = usable.first(options[:maxFiles]) if options[:maxFiles]&.positive?

    usable.each do |fp|
      pair = File.basename(File.dirname(fp))
      tag  = File.basename(fp).split('_merged_cuts_noPmin').first
      leaf = File.join(cfg_dir, pair, tag)
      FileUtils.mkdir_p(leaf)
      info = { 'tfile'=>File.absolute_path(fp), 'ttree'=>VOLATILE_TREE_NAME }
      File.write(File.join(leaf,'tree_info.yaml'), info.to_yaml)
    end

    puts "Prepared #{cfg_dir} (#{usable.size} trees)"
  end
end

puts "\nDirectory tree:"
system('tree', out_root)

# ---------- helper to launch each module ---------------------------
def invoke(name, *args)
  puts "\n=> #{name}: #{args.shelljoin}"
  system(*args) or abort "#{name} failed"
end

asym_ids = []

modules.each do |mod|
  case mod
  when 'filterTree'
      args = ['ruby','./scripts/modules/module___filterTree.rb', project_name]
      args << options[:maxEntries].to_s if options[:maxEntries]
      invoke('filterTree', *args)

  when 'purityBinning'
    invoke('purityBinning',
           'ruby','./scripts/modules/module___purityBinning.rb', project_name)

  when 'asymmetry'
    if options[:is_running_on_slurm]
      # capture the job‐ids that module___asymmetry.rb prints
      out = `ruby ./scripts/modules/module___asymmetry.rb --slurm #{project_name}`
      puts out
      out.each_line.grep(/\[SLURM_JOBS\]/) do |ln|
        asym_ids += ln.split.last.split(/,/)
      end
    else
      invoke('asymmetry',
             'ruby','./scripts/modules/module___asymmetry.rb', project_name)
    end
      
  when 'kinematicBins'
    invoke('kinematicBins',
           'ruby','./scripts/modules/module___kinematicBins.rb', project_name)
      
  when 'baryonContamination'
    invoke('baryonContamination',
           'ruby','./scripts/modules/module___baryonContamination.rb', project_name)

  when 'particleMisidentification'
    invoke('particleMisidentification',
           'ruby','./scripts/modules/module___particleMisidentification.rb', project_name)

  when 'binMigration'
    invoke('binMigration',
           'ruby','./scripts/modules/module___binMigration.rb', project_name)
      
  else
    warn "WARNING: unknown module '#{mod}' – skipped"
  end
end