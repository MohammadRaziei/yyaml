// Source of truth for the yyaml docs site; copied into build/docusaurus during make.
module.exports = {
  title: 'yyaml',
  tagline: 'High-performance YAML parsing in C with C++ bindings',
  url: 'https://mohammadraziei.github.io',
  baseUrl: '/yyaml',
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
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          sidebarPath: './sidebars.js',
          // Please change this to your repo.
          // Remove this to remove the "edit this page" links.
          editUrl:
            'https://github.com/facebook/docusaurus/tree/main/packages/create-docusaurus/templates/shared/',
        },
        blog: {
          showReadingTime: true,
          feedOptions: {
            type: ['rss', 'atom'],
            xslt: true,
          },
          // Please change this to your repo.
          // Remove this to remove the "edit this page" links.
          editUrl:
            'https://github.com/facebook/docusaurus/tree/main/packages/create-docusaurus/templates/shared/',
          // Useful options to enforce blogging best practices
          onInlineTags: 'warn',
          onInlineAuthors: 'warn',
          onUntruncatedBlogPosts: 'warn',
        },
        theme: {
          customCss: './src/css/custom.css',
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      // Replace with your project's social card
      image: 'img/docusaurus-social-card.jpg',
      colorMode: {
        respectPrefersColorScheme: true,
      },
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
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
      },
    }),
};

export default config;
