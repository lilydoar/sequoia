package main

import (
	"encoding/json"
	"fmt"
	"html/template"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
)

const (
	assetOutPath = "assets/gen/atlas"
	codeOutPath  = "src/gen/atlas"
)

var atlases []Atlas = []Atlas{
	// {name: "effects", folders: []string{"Effects"}},
	// {name: "goblins_buildings", folders: []string{"Factions/Goblins/Buildings"}},
	// {name: "goblins_troops", folders: []string{"Factions/Knights/Troops"}},
	// {name: "knights_buildings", folders: []string{"Factions/Goblins/Buildings"}},
	// {name: "knights_troops", folders: []string{"Factions/Knights/Troops"}},
	{name: "resources", folders: []string{"Resources"}},
	// {name: "terrain", folders: []string{"Terrain"}},
	// {name: "ui", folders: []string{"UI"}},
}

type Atlas struct {
	name    string
	folders []string
}

func main() {
	// Reset output directories
	resetDir(assetOutPath)
	resetDir(codeOutPath)

	// Create output directory
	if err := os.RemoveAll(assetOutPath); err != nil {
		fmt.Printf("Error cleaning output directory: %v\n", err)
		os.Exit(1)
	}
	if err := os.MkdirAll(assetOutPath, 0755); err != nil {
		fmt.Printf("Error creating output directory: %v\n", err)
		os.Exit(1)
	}

	// Generate each atlas
	for _, atlas := range atlases {
		if err := generateAtlas(atlas); err != nil {
			fmt.Printf("Error generating atlas %s: %v\n", atlas.name, err)
			os.Exit(1)
		}

		spriteData, err := parseAtlasJson(atlas.name)
		if err != nil {
			fmt.Printf("Error parsing json: %v\n", err)
			os.Exit(1)
		}

		animations := groupFrames(spriteData.Frames, spriteData.Meta.FrameTags)

		if err = generateCode(atlas.name, animations); err != nil {
			fmt.Printf("Error generating code: %v\n", err)
			os.Exit(1)
		}
	}
}

func resetDir(path string) {
	if err := os.RemoveAll(path); err != nil {
		fmt.Printf("Error cleaning output directory: %v\n", err)
		os.Exit(1)
	}
	if err := os.MkdirAll(path, 0755); err != nil {
		fmt.Printf("Error creating output directory: %v\n", err)
		os.Exit(1)
	}
}

func findSpriteFiles(basePath string) (map[string]string, error) {
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
		files, err := findSpriteFiles(basePath)
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
		"--data", filepath.Join(assetOutPath, atlas.name+".json"),
		"--sheet", filepath.Join(assetOutPath, atlas.name+".png"),
		"--sheet-type", "packed",
		"--list-tags",
		"--tagname-format", "{title} {tag}",
		"--merge-duplicates",
	}
	args = append(args, allFiles...)

	// Execute aseprite command
	cmd := exec.Command("aseprite", args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	fmt.Printf("Generating %s atlas...\n", atlas.name)
	return cmd.Run()
}

type Rect struct {
	X int `json:"x"`
	Y int `json:"y"`
	W int `json:"w"`
	H int `json:"h"`
}

type Size struct {
	W int `json:"w"`
	H int `json:"h"`
}

type Frame struct {
	Frame            Rect `json:"frame"`
	Rotated          bool `json:"rotated"`
	Trimmed          bool `json:"trimmed"`
	SpriteSourceSize Rect `json:"spriteSourceSize"`
	SourceSize       Size `json:"sourceSize"`
	Duration         int  `json:"duration"`
}

type FrameTag struct {
	Name      string `json:"name"`
	From      int    `json:"from"`
	To        int    `json:"to"`
	Direction string `json:"direction"`
	Color     string `json:"color"`
}

type Meta struct {
	App       string     `json:"app"`
	Version   string     `json:"version"`
	Image     string     `json:"image"`
	Format    string     `json:"format"`
	Size      Size       `json:"size"`
	Scale     string     `json:"scale"`
	FrameTags []FrameTag `json:"frameTags"`
}

type AsepriteJSON struct {
	Frames map[string]Frame `json:"frames"`
	Meta   Meta             `json:"meta"`
}

func parseAtlasJson(atlasName string) (*AsepriteJSON, error) {
	data, err := os.ReadFile(filepath.Join(assetOutPath, atlasName+".json"))
	if err != nil {
		return nil, err
	}

	var spriteData AsepriteJSON
	if err := json.Unmarshal(data, &spriteData); err != nil {
		return nil, err
	}

	return &spriteData, nil
}

type AnimationFrame struct {
	Name     string
	X        int
	Y        int
	W        int
	H        int
	Duration int
	Index    int
}

type Animation struct {
	Name   string
	Frames []AnimationFrame
}

func groupFrames(frames map[string]Frame, tags []FrameTag) []Animation {
	// First, organize frames by their prefix and index
	prefixGroups := make(map[string]map[int]AnimationFrame)

	for frameName, frame := range frames {
		// Split like "Resources 1.aseprite" into prefix and number
		parts := strings.Split(frameName, " ")
		if len(parts) != 2 {
			fmt.Fprintf(os.Stderr, "Warning: unexpected frame name format: %s\n", frameName)
			continue
		}

		prefix := parts[0]
		indexStr := strings.TrimSuffix(parts[1], ".aseprite")
		index, err := strconv.Atoi(indexStr)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Warning: could not parse frame index from: %s\n", parts[1])
			continue
		}

		// Create prefix group if it doesn't exist
		if _, exists := prefixGroups[prefix]; !exists {
			prefixGroups[prefix] = make(map[int]AnimationFrame)
		}

		// Store frame in appropriate prefix group
		prefixGroups[prefix][index] = AnimationFrame{
			Name:     frameName,
			X:        frame.Frame.X,
			Y:        frame.Frame.Y,
			W:        frame.Frame.W,
			H:        frame.Frame.H,
			Duration: frame.Duration,
			Index:    index,
		}
	}

	// Create animations based on tags
	var animations []Animation
	for _, tag := range tags {
		// Extract prefix from tag name (e.g., "Resources" from "Resources Wood")
		tagPrefix := strings.Split(tag.Name, " ")[0]

		// Find corresponding frame group
		frameGroup, exists := prefixGroups[tagPrefix]
		if !exists {
			fmt.Fprintf(os.Stderr, "Warning: no frames found for prefix: %s\n", tagPrefix)
			continue
		}

		// Collect frames for this tag
		frames := make([]AnimationFrame, 0, tag.To-tag.From+1)
		for i := tag.From; i <= tag.To; i++ {
			if frame, exists := frameGroup[i]; exists {
				frames = append(frames, frame)
			} else {
				fmt.Fprintf(os.Stderr, "Warning: missing frame %d for animation %s\n", i, tag.Name)
			}
		}

		if len(frames) > 0 {
			animations = append(animations, Animation{
				Name:   tag.Name,
				Frames: frames,
			})
		}
	}

	return animations
}

const headerTemplate = `// Generated code -- DO NOT EDIT
// Generated from {{.InputFile}}

#include "atlas.h"

`

const frameArrayTemplate = `static const struct AnimationClip s_{{.Name | sublower}}Clip = {
    .name = "{{.Name}}",
    .frameCount = {{len .Frames}},
    .frames = {
        {{- range .Frames}}
        { .frameName = "{{.Name}}", .rect = { {{.X}}, {{.Y}}, {{.W}}, {{.H}} }, .durationTicks = {{.Duration}} },
        {{- end}}
    }
};

`

const libraryTemplate = `
{{- $libName := .LibraryName -}}
static struct AnimationLibrary g_{{$libName | lower}}AnimLibrary = {
    .clips = {
        {{- range .Clips}}
        s_{{.Name | sublower}}Clip,
        {{- end}}
    },
    .clipCount = {{len .Clips}}
};

{{- range .Clips}}
#define ANIM_{{.Name | subupper}} g_{{$libName | lower}}AnimLibrary.clips[{{.Index}}]
{{- end}}
`

type TemplateFrame struct {
	Name     string
	X        int
	Y        int
	W        int
	H        int
	Duration int
}

type TemplateClip struct {
	Name   string
	Frames []TemplateFrame
	Index  int
}

type TemplateData struct {
	LibraryName string
	InputFile   string
	Clips       []TemplateClip
}

var templateFuncs = template.FuncMap{
	"lower": strings.ToLower,
	"upper": strings.ToUpper,
	"sublower": func(s string) string {
		// Convert "Happy_Sheep Idle" to "happy_sheep_idle"
		s = strings.ReplaceAll(s, " ", "_")
		return strings.ToLower(s)
	},
	"subupper": func(s string) string {
		// Convert "Happy_Sheep Idle" to "HAPPY_SHEEP_IDLE"
		s = strings.ReplaceAll(s, " ", "_")
		return strings.ToUpper(s)
	},
}

func generateCode(atlasName string, animations []Animation) error {
	if len(animations) == 0 {
		return nil
	}

	// Create templates
	headerTmpl := template.Must(template.New("header").Parse(headerTemplate))
	frameTmpl := template.Must(template.New("frame").Funcs(templateFuncs).Parse(frameArrayTemplate))
	libraryTmpl := template.Must(template.New("library").Funcs(templateFuncs).Parse(libraryTemplate))

	var buf strings.Builder

	// Convert animations to template format
	clips := make([]TemplateClip, len(animations))
	for i, anim := range animations {
		frames := make([]TemplateFrame, len(anim.Frames))
		for j, frame := range anim.Frames {
			frames[j] = TemplateFrame{
				Name:     frame.Name,
				X:        frame.X,
				Y:        frame.Y,
				W:        frame.W,
				H:        frame.H,
				Duration: convertMsToTicks(frame.Duration),
			}
		}

		clips[i] = TemplateClip{
			Name:   anim.Name,
			Frames: frames,
			Index:  i,
		}
	}

	data := TemplateData{
		LibraryName: atlasName,
		InputFile:   filepath.Join(assetOutPath, atlasName+".json"),
		Clips:       clips,
	}

	// Execute templates
	if err := headerTmpl.Execute(&buf, data); err != nil {
		return fmt.Errorf("header template error: %w", err)
	}

	// Generate each animation clip
	for _, clip := range clips {
		if err := frameTmpl.Execute(&buf, clip); err != nil {
			return fmt.Errorf("frame template error for %s: %w", clip.Name, err)
		}
	}

	// Generate library
	if err := libraryTmpl.Execute(&buf, data); err != nil {
		return fmt.Errorf("library template error: %w", err)
	}

	// Create output directory
	outFile := filepath.Join(codeOutPath, atlasName+".atlas.c")

	dir := filepath.Dir(outFile)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return fmt.Errorf("error creating output directory %s: %w", dir, err)
	}

	// Write to file
	if err := os.WriteFile(outFile, []byte(buf.String()), 0644); err != nil {
		return fmt.Errorf("error writing output file: %w", err)
	}

	return nil
}

const ticksPerSec = 60

func convertMsToTicks(ms int) int {
	return ms * ticksPerSec / 1000
}
