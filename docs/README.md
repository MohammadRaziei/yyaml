# YYAML Documentation

This directory contains the documentation for the yyaml YAML parser library.

## Building Documentation Locally

```bash
cd docs
make build
```

This will generate the documentation in `docs/build/output-html/`.

## Serving Documentation Locally

```bash
cd docs
make serve
```

This will serve the documentation on `http://localhost:3000`.

## GitHub Pages Deployment

The documentation is automatically deployed to GitHub Pages via the GitHub Actions workflow in `.github/workflows/docs.yml`.

### Setting up GitHub Pages

1. Go to your repository settings on GitHub
2. Navigate to "Pages" in the sidebar
3. Under "Build and deployment", select "GitHub Actions" as the source
4. The workflow will automatically deploy to GitHub Pages on pushes to the `master` branch

### Custom Domain (Optional)

If you want to use a custom domain:

1. Create a `CNAME` file in the `docs/static/` directory with your domain name
2. Configure your DNS settings to point to GitHub Pages

## Documentation Structure

- `index.mdx` - Main landing page
- `languages/` - Language-specific documentation (C and C++)
- `benchmarks/` - Performance benchmarks
- `src/` - Docusaurus source files
- `static/` - Static assets (images, etc.)
- `.docsignore` - Files to exclude during documentation build

## API Documentation

The API documentation is generated using:
- **Doxygen** - Extracts XML from source code
- **Doxybook2** - Converts XML to Markdown
