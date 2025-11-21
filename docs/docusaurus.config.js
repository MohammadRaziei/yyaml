// Source of truth for the yyaml docs site; copied into build/docusaurus during make.
module.exports = {
  title: 'yyaml',
  tagline: 'High-performance YAML parsing in C with C++ bindings',
  url: 'https://example.com',
  baseUrl: '/',
  trailingSlash: false,
  onBrokenLinks: 'warn',
  onBrokenMarkdownLinks: 'warn',
  organizationName: 'yyaml',
  projectName: 'yyaml-docs',
  staticDirectories: [],
  markdown: {
    format: 'mdx',
  },
  presets: [
    [
      'classic',
      {
        docs: {
          path: '.',
          routeBasePath: '/',
          sidebarPath: require.resolve('./sidebars.js'),
          include: ['**/*.{md,mdx}'],
          breadcrumbs: true,
          editUrl: undefined,
        },
        blog: false,
        pages: false,
        theme: {
          customCss: [],
        },
      },
    ],
  ],
  themeConfig: {
    navbar: {
      title: 'yyaml',
      items: [
        { to: '/', label: 'Overview', position: 'left' },
        { to: '/languages', label: 'Languages', position: 'left' },
        { to: '/benchmarks', label: 'Benchmarks', position: 'left' },
        { href: 'https://github.com/mohammadraziei/yyaml', label: 'GitHub', position: 'right' },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Docs',
          items: [
            { label: 'Overview', to: '/' },
            { label: 'C API', to: '/languages/c' },
            { label: 'C++ API', to: '/languages/cpp' },
          ],
        },
        {
          title: 'Community',
          items: [
            { label: 'GitHub Issues', href: 'https://github.com/mohammadraziei/yyaml/issues' },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} yyaml.`,
    },
    prism: {
      theme: require('prism-react-renderer/themes/github'),
      darkTheme: require('prism-react-renderer/themes/dracula'),
    },
  },
};
