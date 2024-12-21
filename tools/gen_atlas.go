package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

const outPath string = "assets/gen/atlas"

var atlases []Atlas = []Atlas{
	{name: "deco", folders: []string{"Deco"}},
	{name: "effects", folders: []string{"Effects"}},
	{name: "goblins_buildings", folders: []string{"Factions/Goblins/Buildings"}},
	{name: "goblins_troops", folders: []string{"Factions/Knights/Troops"}},
	{name: "knights_buildings", folders: []string{"Factions/Goblins/Buildings"}},
	{name: "knights_troops", folders: []string{"Factions/Knights/Troops"}},
	{name: "resources", folders: []string{"Resources"}},
	{name: "terrain", folders: []string{"Terrain"}},
	{name: "ui", folders: []string{"UI"}},
}

type Atlas struct {
	name    string
	folders []string
}

func main() {
	// Create output directory
	if err := os.RemoveAll(outPath); err != nil {
		fmt.Printf("Error cleaning output directory: %v\n", err)
		os.Exit(1)
	}
	if err := os.MkdirAll(outPath, 0755); err != nil {
		fmt.Printf("Error creating output directory: %v\n", err)
		os.Exit(1)
	}

	// Generate each atlas
	for _, atlas := range atlases {
		if err := generateAtlas(atlas); err != nil {
			fmt.Printf("Error generating atlas %s: %v\n", atlas.name, err)
			os.Exit(1)
		}
	}
}

func findFiles(basePath string) (map[string]string, error) {
	// key: filepath without extension, value: actual filepath
	files := make(map[string]string)

	err := filepath.Walk(basePath, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if info.IsDir() {
			return nil
		}

		ext := strings.ToLower(filepath.Ext(path))
		if ext != ".aseprite" && ext != ".png" {
			return nil
		}

		// Get the file path without extension
		basePathWithoutExt := strings.TrimSuffix(path, ext)

		// If we find an aseprite file, it takes precedence
		if ext == ".aseprite" {
			files[basePathWithoutExt] = path
		} else if ext == ".png" {
			// Only add PNG if there isn't already an aseprite file
			if _, exists := files[basePathWithoutExt]; !exists {
				files[basePathWithoutExt] = path
			}
		}

		return nil
	})

	return files, err
}

func generateAtlas(atlas Atlas) error {
	var allFiles []string

	// Collect files from each folder
	for _, folder := range atlas.folders {
		basePath := filepath.Join("assets/sprites", folder)
		files, err := findFiles(basePath)
		if err != nil {
			return fmt.Errorf("error walking directory %s: %v", folder, err)
		}

		// Add all collected files to our list
		for _, file := range files {
			allFiles = append(allFiles, file)
		}
	}

	// Skip if no files found
	if len(allFiles) == 0 {
		fmt.Printf("No files found for %s atlas\n", atlas.name)
		return nil
	}

	// Prepare aseprite command
	args := []string{
		"--batch",
		"--data", filepath.Join(outPath, atlas.name+".json"),
		"--sheet", filepath.Join(outPath, atlas.name+".png"),
		"--sheet-type", "packed",
		// "--sheet-width", "4096",
		// "--sheet-height", "4096",
	}
	args = append(args, allFiles...)

	// Execute aseprite command
	cmd := exec.Command("aseprite", args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	fmt.Printf("Generating %s atlas...\n", atlas.name)
	return cmd.Run()
}
